#include <psp2/kernel/sysmem.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/io/stat.h>
#include <psp2/ctrl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
/* v2.1 user syscall ABI. The upstream SDK header includes official
 * SDK-only scetypes.h; these three C signatures need no SDK typedefs. */
extern int vnzBridgeInject(const void *,unsigned int);
extern int vnzBridgeExec(void *,unsigned int);
extern int vnzBridgeRestore(void);
#include "probe.h"
#include "elfcheck.h"

#define RUNS 512
static FILE *logfile;
static uint64_t timings[RUNS];
static void report(const char *fmt,...) {
    va_list ap; va_start(ap,fmt);
    vfprintf(logfile,fmt,ap);va_end(ap);fflush(logfile);
}
static int cmp(const void *a,const void *b) {
    uint64_t x=*(const uint64_t *)a,y=*(const uint64_t *)b;
    return (x>y)-(x<y);
}
static void stats(const char *name,unsigned n) {
    if(!n)return;
    qsort(timings,n,sizeof(timings[0]),cmp);
    report("[VNZProbe] case=%s n=%u p50Us=%llu p95Us=%llu p99Us=%llu maxUs=%llu\n",
        name,n,(unsigned long long)timings[(n-1)*50/100],
        (unsigned long long)timings[(n-1)*95/100],
        (unsigned long long)timings[(n-1)*99/100],(unsigned long long)timings[n-1]);
}
static ProbePacket packet(uint32_t command,uint32_t serial,uint32_t rounds) {
    ProbePacket p={PROBE_MAGIC,PROBE_ABI,command,serial,0x12345678u^serial,rounds,
        PROBE_UNTOUCHED,PROBE_UNTOUCHED,PROBE_UNTOUCHED,0xa5c39e17u};
    return p;
}
static int run_case(const char *name,uint32_t command,uint32_t rounds,unsigned n) {
    for(unsigned i=0;i<n;++i){
        ProbePacket p=packet(command,i+1,rounds),expected=p;
        if(probe_handle(&expected,sizeof(expected)))return -1;
        uint64_t begin=sceKernelGetProcessTimeWide();
        int rc=vnzBridgeExec(&p,sizeof(p));
        timings[i]=sceKernelGetProcessTimeWide()-begin;
        if(rc!=0 || memcmp(&p,&expected,sizeof(p))){
            report("[VNZProbe] FAIL case=%s i=%u rc=0x%08X result=%08X expected=%08X status=%u serial=%u guard=%08X\n",
                name,i,(unsigned)rc,p.result,expected.result,p.status,p.response_serial,p.guard);
            return -1;
        }
    }
    stats(name,n);return 0;
}
static int run_probe(void) {
    int result=-1,injected=0;
    SceUID block=-1;
    void *mem=NULL;
    FILE *elf=fopen("app0:mep_probe.elf","rb");
    if(!elf){report("[VNZProbe] missing ELF\n");return -1;}
    if(fseek(elf,0,SEEK_END))goto done;
    long size=ftell(elf);
    if(size<768 || size>PROBE_ELF_LIMIT || fseek(elf,0,SEEK_SET))goto done;
    block=sceKernelAllocMemBlock("LagiVnzCode",SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW,PROBE_ELF_LIMIT,NULL);
    if(block<0){report("[VNZProbe] alloc rc=0x%08X\n",(unsigned)block);goto done;}
    int rc=sceKernelGetMemBlockBase(block,&mem);
    if(rc<0 || !mem){report("[VNZProbe] base rc=0x%08X\n",(unsigned)rc);goto done;}
    memset(mem,0,PROBE_ELF_LIMIT);
    if(fread(mem,1,(size_t)size,elf)!=(size_t)size || !probe_elf_check(mem,(size_t)size)){
        report("[VNZProbe] FAIL ELF validation size=%ld\n",size);goto done;
    }
    report("[VNZProbe] ELF validated bytes=%ld entry=0x%08X\n",size,probe_u32((unsigned char*)mem+24));
    fclose(elf);elf=NULL;
    report("[VNZProbe] inject begin\n");
    uint64_t begin=sceKernelGetProcessTimeWide();
    rc=vnzBridgeInject(mem,PROBE_ELF_LIMIT);
    report("[VNZProbe] inject rc=0x%08X us=%llu\n",(unsigned)rc,(unsigned long long)(sceKernelGetProcessTimeWide()-begin));
    if(rc!=0){
        /* A failed upstream inject may leave a kernel mapping. Do not free
         * the code block explicitly or attempt a second injection. */
        block=-1;goto done;
    }
    injected=1;
    report("[VNZProbe] first guarded RPC begin\n");
    if(run_case("cold_echo",PROBE_ECHO,0,1) ||
       run_case("warm_echo",PROBE_ECHO,0,RUNS) ||
       run_case("math_256",PROBE_MATH,256,RUNS) ||
       run_case("math_4096",PROBE_MATH,4096,RUNS) ||
       run_case("bad_command",99,0,1) ||
       run_case("bad_rounds",PROBE_MATH,PROBE_MAX_ROUNDS+1,1))goto done;
    result=0;
done:
    if(elf)fclose(elf);
    if(injected){
        report("[VNZProbe] restore begin\n");
        uint64_t t=sceKernelGetProcessTimeWide();
        int rc=vnzBridgeRestore();
        report("[VNZProbe] restore rc=0x%08X us=%llu\n",(unsigned)rc,(unsigned long long)(sceKernelGetProcessTimeWide()-t));
        if(rc!=0){result=-1;block=-1;} // retain allocation until process teardown
    }
    if(block>=0){
        int rc=sceKernelFreeMemBlock(block);
        if(rc<0){report("[VNZProbe] free rc=0x%08X\n",(unsigned)rc);result=-1;}
    }
    return result;
}
int main(void) {
    sceIoMkdir("ux0:data/lagi",0777);
    logfile=fopen("ux0:data/lagi/venezia_probe.log","w");
    if(!logfile)return 1;
    report("[VNZProbe] abi=1 release=v2.1 standalone; CROSS to run, CIRCLE to cancel\n");
    /* Explicit launch consent; no automatic injection during normal game startup. */
    SceCtrlData pad;int released=0;
    for(;;){
        memset(&pad,0,sizeof(pad));sceCtrlPeekBufferPositive(0,&pad,1);
        if(!(pad.buttons&(SCE_CTRL_CROSS|SCE_CTRL_CIRCLE)))released=1;
        if(released && (pad.buttons&SCE_CTRL_CIRCLE)){report("[VNZProbe] canceled\n");fclose(logfile);return 0;}
        if(released && (pad.buttons&SCE_CTRL_CROSS))break;
        sceKernelDelayThread(16000);
    }
    int rc=run_probe();
    report("[VNZProbe] %s\n",rc==0?"PASS":"FAIL; reboot before retry after bridge error");
    fclose(logfile);
    return rc?1:0;
}

#include "probe.h"
#include "elfcheck.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void put32(unsigned char *p,uint32_t x){
    for(int i=0;i<4;i++)p[i]=(unsigned char)(x>>(i*8));
}
int main(void){
    ProbePacket p={PROBE_MAGIC,PROBE_ABI,PROBE_ECHO,42,123,0,0,PROBE_UNTOUCHED,0,0xa5c39e17u};
    assert(!probe_handle(&p,sizeof(p)) && p.result==123 && p.response_serial==42 && p.status==0);
    assert(p.guard==0xa5c39e17u);
    p.command=PROBE_MATH;p.seed=1;p.rounds=1;
    assert(!probe_handle(&p,sizeof(p)) && p.result==270369u);
    p.rounds=PROBE_MAX_ROUNDS+1;
    assert(!probe_handle(&p,sizeof(p)) && p.status==PROBE_BAD_ROUNDS);
    p.command=99;assert(!probe_handle(&p,sizeof(p)) && p.status==PROBE_BAD_COMMAND);
    assert(probe_handle(&p,sizeof(p)-1)<0);
    p.magic=0;assert(probe_handle(&p,sizeof(p))<0);
    unsigned char e[1024]={0};
    memcpy(e,"\x7f" "ELF\x01\x01\x01",7);e[16]=2;e[18]=0x0d;e[19]=0xf0;e[20]=1;e[40]=52;e[46]=40;e[48]=2;
    put32(e+24,768);put32(e+32,800);
    put32(e+840+4,1);put32(e+840+8,6);put32(e+840+12,768);put32(e+840+16,768);put32(e+840+20,16);
    assert(probe_elf_check(e,sizeof(e)));
    put32(e+840+16,256);assert(!probe_elf_check(e,sizeof(e)));
    puts("PASS: packet semantics and ELF mailbox/entry validation");
}

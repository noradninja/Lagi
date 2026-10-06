/* Included against mocked Vita APIs by tools/run_scspdsp_vm_tests.py.
 * The probe's ARM leaf and published ARM code actually execute in Unicorn. */
#include "lagi_dsp_platform_vita.cpp"
static unsigned calls[6];
static int failOperation=-1, failCall=1, wrongReturn=0;
static unsigned char memory[262144] __attribute__((aligned(4096)));
static const char *setting="";
static int operation(unsigned op) { ++calls[op]; return failOperation==(int)op && calls[op]==(unsigned)failCall ? -1234 : 0; }
extern "C" SceUID sceKernelAllocMemBlockForVM(const char *,SceSize) { return operation(0)<0?-1234:1; }
extern "C" int sceKernelGetMemBlockBase(SceUID,void **base) { *base=memory; return operation(1); }
extern "C" int sceKernelOpenVMDomain() { return operation(2); }
extern "C" int sceKernelCloseVMDomain() { return operation(3); }
extern "C" int sceKernelSyncVMDomain(SceUID,void *,SceSize) { if(wrongReturn) ((uint32_t*)memory)[0]=0xe3a00029; return operation(4); }
extern "C" int sceKernelFreeMemBlock(SceUID) { return operation(5); }
extern "C" SceUID sceIoOpen(const char *path,int,int) { return std::strcmp(path,"ux0:data/lagi/dsp_backend.txt")? -1 : 10; }
extern "C" int sceIoRead(SceUID,void *data,SceSize size) { unsigned n=0; while(setting[n] && n<size) ++n; std::memcpy(data,setting,n); return n; }
extern "C" int sceIoWrite(SceUID,const void *,SceSize size) { return size; }
extern "C" int sceIoClose(SceUID) { return 0; }
extern "C" int sceIoRemove(const char *) { return 0; }
extern "C" int sceIoMkdir(const char *,int) { return 0; }
extern "C" void lagi_dsp_release_native() {}
namespace std {
int vsnprintf(char *buffer,size_t size,const char *,va_list) { if(size) buffer[0]=0; return 0; }
int snprintf(char *buffer,size_t size,const char *,...) { if(size) buffer[0]=0; return 0; }
}
namespace lagi { namespace platform { namespace logging { void writef(const char *,...) {} } } }
static void reset(int fail=-1,int nth=1) {
    initialized=false; vmReady=false; mode=LAGI_DSP_PREDECODED; pool=-1; poolBase=0;
    for(unsigned i=0;i<6;++i) calls[i]=0;
    failOperation=fail; failCall=nth; wrongReturn=0; setting="";
}
int main() {
    for(int op=0;op<6;++op) {
        reset(op);
        lagi_dsp_platform_init();
        if(lagi_dsp_vm_available()) return 10+op;
        if(calls[5]!=(op==0?0u:1u)) return 20+op;
        if(op==3 && calls[3]!=2) return 30;
    }
    reset(); wrongReturn=1; lagi_dsp_platform_init(); if(lagi_dsp_vm_available()) return 31;
    reset(); setting="arm"; lagi_dsp_platform_init(); if(!lagi_dsp_vm_available()) return 32;
    const uint32_t code[]={0xe3a00007,0xe12fff1e};
    if(lagi_dsp_vm_publish(4,code,8) || lagi_dsp_vm_publish(0,code,65540) || lagi_dsp_vm_publish(0,code,0)) return 33;
    if(calls[0]!=2) return 34;
    void *fn=lagi_dsp_vm_publish(0,code,8);
    if(!fn || ((int(*)(void))fn)()!=7) return 35;
    lagi_dsp_platform_shutdown(); if(calls[5]!=2 || pool!=-1) return 36;
    /* Each pool publication failure disables the backend and can be freed. */
    for(int op=0;op<5;++op) {
        reset(op,2); setting="arm"; lagi_dsp_platform_init();
        if(lagi_dsp_vm_publish(0,code,8) || lagi_dsp_vm_available()) return 40+op;
        lagi_dsp_platform_shutdown(); if(calls[5]!= (op==0?1u:2u)) return 50+op;
    }
    const char *settings[]={"reference\n","auto\r\n","aot","arm","predecoded","invalid"};
    const int modes[]={LAGI_DSP_REFERENCE,LAGI_DSP_AUTO,LAGI_DSP_AOT,LAGI_DSP_ARM,LAGI_DSP_PREDECODED,LAGI_DSP_PREDECODED};
    for(unsigned i=0;i<6;++i) { reset(); setting=settings[i]; if(lagi_dsp_platform_init()!=modes[i]) return 60+i; }
    return 0;
}

"""Probe/publish failure-path tests with mocked Vita APIs and actual ARM execution."""
from pathlib import Path
import os, subprocess, tempfile
from dsp_arm_test_support import run_arm_elf

repo=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='lagi-dsp-vm-test-') as directory:
    out=Path(directory)
    for d in ['psp2/kernel','psp2/io','lagi']: (out/d).mkdir(parents=True,exist_ok=True)
    (out/'lagi/platform.h').write_text('namespace lagi { namespace platform { namespace logging { void writef(const char*,...); } } }')
    (out/'cstdio').write_text('#include <cstddef>\n#include <cstdarg>\nnamespace std { int vsnprintf(char*,size_t,const char*,va_list); int snprintf(char*,size_t,const char*,...); }')
    common='typedef int SceUID; typedef unsigned SceSize;\n'
    (out/'psp2/kernel/sysmem.h').write_text(common+'''extern "C" {
SceUID sceKernelAllocMemBlockForVM(const char*,SceSize);
int sceKernelGetMemBlockBase(SceUID,void**);
int sceKernelOpenVMDomain(); int sceKernelCloseVMDomain();
int sceKernelSyncVMDomain(SceUID,void*,SceSize); int sceKernelFreeMemBlock(SceUID);
}''')
    (out/'psp2/io/fcntl.h').write_text(common+'''#define SCE_O_RDONLY 1
#define SCE_O_WRONLY 2
#define SCE_O_CREAT 4
#define SCE_O_EXCL 8
extern "C" {
SceUID sceIoOpen(const char*,int,int); int sceIoRead(SceUID,void*,SceSize);
int sceIoWrite(SceUID,const void*,SceSize); int sceIoClose(SceUID); int sceIoRemove(const char*);
}''')
    (out/'psp2/io/stat.h').write_text('extern "C" int sceIoMkdir(const char*,int);')
    (out/'lib.cpp').write_text('''#include <stddef.h>
extern "C" {
void *memset(void *p,int c,size_t n) { unsigned char *s=(unsigned char*)p; while(n--) *s++=c; return p; }
void *memcpy(void *p,const void *q,size_t n) { unsigned char *d=(unsigned char*)p; const unsigned char *s=(const unsigned char*)q; while(n--) *d++=*s++; return p; }
int memcmp(const void *p,const void *q,size_t n) { const unsigned char *a=(const unsigned char*)p,*b=(const unsigned char*)q; while(n--) { if(*a!=*b) return 1; ++a; ++b; } return 0; }
int strcmp(const char *a,const char *b) { while(*a && *a==*b) { ++a; ++b; } return (unsigned char)*a-(unsigned char)*b; }
size_t strcspn(const char *s,const char *reject) { size_t n=0; while(s[n]) { for(const char *p=reject;*p;++p) if(s[n]==*p) return n; ++n; } return n; }
}''')
    gcc=Path(os.environ.get('VITASDK',r'C:\Dev\VitaSDK'))/'bin/arm-vita-eabi-g++.exe'
    subprocess.run([str(gcc),'-O2','-marm','-march=armv7-a','-fno-exceptions','-fno-rtti','-ffreestanding','-fno-builtin','-nostdlib','-I'+str(out),'-I'+str(repo/'src/integration'),str(repo/'tests/scspdsp_vm_tests.cpp'),str(out/'lib.cpp'),'-Wl,-Ttext=0x10000','-Wl,-e,main','-lgcc','-o',str(out/'test.elf')],check=True)
    run_arm_elf(out/'test.elf')
print('PASS: 1 MiB single-allocation VM probe/reuse, publication and cleanup failures; real ARM return values; backend settings')

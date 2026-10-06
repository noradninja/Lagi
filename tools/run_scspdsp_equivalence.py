# ARM differential verification; requires Python unicorn and pyelftools.
from pathlib import Path
import os, re, subprocess, tempfile

base = Path(__file__).resolve().parent
from unicorn import Uc, UC_ARCH_ARM, UC_MODE_ARM, UC_HOOK_CODE
from unicorn.arm_const import UC_ARM_REG_SP, UC_ARM_REG_LR, UC_ARM_REG_R0, UC_ARM_REG_C1_C0_2, UC_ARM_REG_FPEXC, UC_ARM_REG_PC
from elftools.elf.elffile import ELFFile

repo = base.parent
temp = tempfile.TemporaryDirectory(prefix='lagi-dsp-test-')
out = Path(temp.name)
(out / 'psp2/kernel').mkdir(parents=True, exist_ok=True)
header = (repo / 'extern/Azel/ThirdParty/aosdk/eng_ssf/scsp.h').read_text()
struct = re.search(r'struct _SCSPDSP\s*\{.*?\n\};', header, re.S).group(0)
(out / 'scsp.h').write_text('''#include <stdint.h>
typedef uint8_t UINT8; typedef uint16_t UINT16; typedef uint32_t UINT32;
typedef int16_t INT16; typedef int32_t INT32; typedef int64_t INT64;
''' + struct)
for f in ['ao.h', 'cpuintrf.h']:
    (out / f).write_text('')
(out / 'psp2/kernel/threadmgr.h').write_text('static unsigned long long sceKernelGetSystemTimeWide(void) { return 0; }')
source = (repo / 'tests/scspdsp_equivalence.c').read_text().replace('fprintf(stderr,', 'printf(')
(out / 'test.c').write_text(source)
(out / 'lib.c').write_text('''#include <stddef.h>
void *memset(void *p,int c,size_t n) { unsigned char *s=p; while(n--) *s++=c; return p; }
void *memcpy(void *p,const void *q,size_t n) { unsigned char *d=p; const unsigned char *s=q; while(n--) *d++=*s++; return p; }
int memcmp(const void *p,const void *q,size_t n) { const unsigned *a=p,*b=q; while(n>=4) { if(*a!=*b) return 1; ++a; ++b; n-=4; } const unsigned char *x=(const unsigned char *)a,*y=(const unsigned char *)b; while(n--) { if(*x!=*y) return 1; ++x; ++y; } return 0; }
int puts(const char *s) { return 0; }
int printf(const char *s,...) { return 0; }
void exit(int status) { for(;;) {} }
void __assert_func(const char *f,int n,const char *fn,const char *e) { exit(99); }
''')
subprocess.run([str(Path(os.environ.get('VITASDK', r'C:\Dev\VitaSDK')) / 'bin/arm-vita-eabi-gcc.exe'), '-O2', '-marm', '-march=armv7-a', '-ffreestanding', '-fno-builtin', '-nostdlib', '-I'+str(out), '-I'+str(repo / 'src/integration'), str(out/'test.c'), str(out/'lib.c'), '-Wl,-Ttext=0x10000', '-Wl,-e,main', '-lgcc', '-o', str(out/'test.elf')], check=True)
uc = Uc(UC_ARCH_ARM, UC_MODE_ARM)
uc.mem_map(0x10000, 0x1000000)
with (out/'test.elf').open('rb') as f:
    elf = ELFFile(f)
    for seg in elf.iter_segments():
        if seg['p_type'] == 'PT_LOAD': uc.mem_write(seg['p_vaddr'], seg.data())
    syms = {s.name:s['st_value'] for s in elf.get_section_by_name('.symtab').iter_symbols()}
uc.reg_write(UC_ARM_REG_SP, 0x1000000)
uc.reg_write(UC_ARM_REG_LR, 0xffff00)
uc.reg_write(UC_ARM_REG_C1_C0_2, 0xf << 20)
uc.reg_write(UC_ARM_REG_FPEXC, 1 << 30)
def stop_on_exit(uc, address, size, unused):
    if address == syms['exit']:
        raise RuntimeError('Differential test failed: status=' + str(uc.reg_read(UC_ARM_REG_R0)))
uc.hook_add(UC_HOOK_CODE, stop_on_exit, begin=syms['exit'], end=syms['exit'])
def stop_on_assert(uc, address, size, unused):
    raise RuntimeError('DSP assertion failed')
uc.hook_add(UC_HOOK_CODE, stop_on_assert, begin=syms['__assert_func'], end=syms['__assert_func'])
uc.emu_start(syms['main'], 0xffff00, count=10000000000)
assert uc.reg_read(UC_ARM_REG_PC) == 0xffff00, 'Instruction limit reached'
assert uc.reg_read(UC_ARM_REG_R0) == 0
print('PASS: 1,024 ARM 84-step fast/generic comparisons and all fallback guards')

# ARM differential verification; requires Python unicorn and pyelftools.
from pathlib import Path
import argparse, os, random, re, subprocess, tempfile
from generate_scspdsp_aot import generate, load_corpus

base = Path(__file__).resolve().parent
from dsp_arm_test_support import run_arm_elf

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
# Full-ISA synthetic corpus covers 52, 84 and 128 steps. Captured banks, if
# present, are appended automatically rather than requiring scene-specific tests.
parser=argparse.ArgumentParser(description="ARM DSP translator differential tests")
parser.add_argument('--corpus',type=Path,default=repo/'dsp_programs')
parser.add_argument('--synthetic-count',type=int,default=32,choices=range(1,33))
parser.add_argument('--thumb',action='store_true',help='Compile reference/C/helpers in Thumb mode, matching Vita defaults')
args=parser.parse_args()
rng=random.Random(0x597fb08)
programs=[]
for p in range(args.synthetic_count):
    words=[]
    for i in range([52,84,128][p%3]):
        w0=rng.randrange(0x8000)
        ira=rng.randrange(50)
        iwa=rng.randrange(32)
        if i%7==0: ira=iwa
        w1=(rng.randrange(2)<<15)|(rng.randrange(4)<<13)|(ira<<6)|(rng.randrange(2)<<5)|iwa
        w2=rng.randrange(65536); w3=rng.randrange(65536)
        if p%4==0:
            w1=(w1 & ~(3<<13)) | (rng.randrange(2)<<13)
            w2=(w2 & ~0xf8) | (rng.randrange(2)<<4)
            w3 &= 0x7fff
        words.extend([w0,w1,w2,w3])
    words[-4] |= 0x80
    programs.append(tuple(words))
captured=load_corpus(args.corpus)
programs+=captured
(out/'lagi_dsp_aot.inc').write_text(generate(programs,repo/'src/integration/lagi_dsp_fields.def'))
print(f'Testing {len(programs)} native programs, including {len(captured)} captured programs',flush=True)
source = (repo / 'tests/scspdsp_equivalence.c').read_text().replace('fprintf(stderr,', 'printf(')
(out / 'test.c').write_text(source)
(out / 'lib.c').write_text('''#include <stddef.h>
void *memset(void *p,int c,size_t n) { unsigned char *s=p; while(n--) *s++=c; return p; }
void *memcpy(void *p,const void *q,size_t n) { unsigned char *d=p; const unsigned char *s=q; while(n--) *d++=*s++; return p; }
int memcmp(const void *p,const void *q,size_t n) { const unsigned *a=p,*b=q; while(n>=4) { if(*a!=*b) return 1; ++a; ++b; n-=4; } const unsigned char *x=(const unsigned char *)a,*y=(const unsigned char *)b; while(n--) { if(*x!=*y) return 1; ++x; ++y; } return 0; }
int strcmp(const char *a,const char *b) { while(*a && *a==*b) { ++a; ++b; } return (unsigned char)*a-(unsigned char)*b; }
int puts(const char *s) { return 0; }
int printf(const char *s,...) { return 0; }
void exit(int status) { for(;;) {} }
void __assert_func(const char *f,int n,const char *fn,const char *e) { exit(99); }
''')
subprocess.run([str(Path(os.environ.get('VITASDK', r'C:\Dev\VitaSDK')) / 'bin/arm-vita-eabi-gcc.exe'), '-DLAGI_DSP_TEST', '-O2', '-mthumb' if args.thumb else '-marm', '-march=armv7-a', '-ffreestanding', '-fno-builtin', '-nostdlib', '-I'+str(out), '-I'+str(repo / 'src/integration'), str(out/'test.c'), str(out/'lib.c'), '-Wl,-Ttext=0x10000', '-Wl,-e,main', '-lgcc', '-o', str(out/'test.elf')], check=True)
run_arm_elf(out/'test.elf')
print(f'PASS ({"Thumb" if args.thumb else "ARM"} helpers): 1,024 predecoded comparisons; {len(programs)*16} samples compared across reference, generated C, generated ARM; selection/invalidation/failure tests')

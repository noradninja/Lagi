from pathlib import Path
import struct,sys,re,subprocess,tempfile
r=Path(__file__).resolve().parents[1]
b=Path(sys.argv[1]).read_bytes();base=0x6054000
u=lambda a:struct.unpack_from('>I',b,a-base)[0]
for ea,word in [(0x605ED54,0x2008),(0x605ED56,0x8B01),(0x605ED5A,0xE001),(0x605ED5C,0xE000),(0x605ED72,0x2008),(0x605ED74,0x891D)]:
 assert struct.unpack_from('>H',b,ea-base)[0]==word
rows=[];p=0x609C678
while True:
 threshold,interval=u(p),u(p+4);assert interval>0;rows.append((threshold,interval));p+=8
 if threshold>=255:break
source=(r/'src/integration/lagi_oasis_sand_pits.inc').read_text()
condition=re.search(r'if \((field->m18_wormColorState1\+\+ % readSaturnU32\(row \+ 4\) [!=]= 0)\)',source).group(1)
fixture=r"""
#include <cassert>
struct Field{unsigned m18_wormColorState1;}f,*field=&f;
unsigned interval;unsigned row=0;unsigned readSaturnU32(unsigned){return interval;}
bool selected(){if(CONDITION)return false;return true;}
int main(){for(interval=1;interval<=112;++interval){f.m18_wormColorState1=0;for(unsigned n=0;n<224;++n)assert(selected()==(n%interval==0));}}
""".replace('CONDITION',condition)
with tempfile.TemporaryDirectory() as d:
 p=Path(d);(p/'test.cpp').write_text(fixture)
 subprocess.run(['cl','/nologo','/EHsc',str(p/'test.cpp'),'/Fe:'+str(p/'test.exe'),'/Fo:'+str(p/'test.obj')],check=True)
 subprocess.run([str(p/'test.exe')],check=True)
print('PASS native selector return and caller branch; production condition across 112 intervals and 224 placements; authored rows',rows)

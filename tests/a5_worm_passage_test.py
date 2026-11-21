"""Exercise production passage init/unlock/selection/transition with original A5 records."""
from pathlib import Path
import re,subprocess,sys,tempfile
r=Path(__file__).resolve().parents[1]
s=(r/'extern/Azel/AzelLib/field/field_a5/a5_wormPassageEntrance.cpp').read_text()
c=(r/'CMakeLists.txt').read_text()
for name in ['fields','decode','callback','target','normal','debris','draw_bit','init_bit','gate1','gate2']:
 name='a5_passage_'+name
 def binding(suffix):return re.search(r'set\('+name+suffix+r' \[=\[(.*?)\]=\]\)',c,re.S).group(1)
 old,new=binding('_old'),binding('_new');assert old in s;s=s.replace(old,new)
def body(s,name):
 a=s.index(name);a=s.rfind('\n',0,a)+1;b=s.index('{',a);depth=1;end=b+1
 while depth:depth+=(s[end]=='{')-(s[end]=='}');end+=1
 return s[a:end]
common=(r/'extern/Azel/AzelLib/common.h').read_text()
state='\n'.join(body(common,n) for n in ['bool getBit(u32 bitIndex)','void setBit(u32 bitIndex)','bool getBit566(u32 bitIndex)','void setBit566(u32 bitIndex)'])
a=s.index('struct sA5WormPassageEntrance :');b=s.index('};',a)+2
parts=[s[a:b]+';',(r/'src/integration/lagi_a5_worm_passage_selected.inc').read_text()]
for name in ['runUnlockTransition','DrawWithModel','Init','Update']:
 parts.append(body(s,'static void a5WormPassageEntrance_'+name+'('))
f=(r/'tests/a5_worm_passage_test.cpp').read_text().replace('// PRODUCTION_BITS',state).replace('// PRODUCTION_PASSAGE','\n'.join(parts))
with tempfile.TemporaryDirectory(prefix='lagi-a5-passage-') as d:
 p=Path(d);(p/'test.cpp').write_text(f)
 subprocess.run(['cl','/nologo','/EHsc','/std:c++20','/I'+str(r/'extern/Azel/AzelLib'),str(p/'test.cpp'),'/Fe:'+str(p/'test.exe'),'/Fo:'+str(p/'test.obj')],check=True)
 subprocess.run([str(p/'test.exe'),sys.argv[1]],check=True)
p=Path(sys.argv[1]).read_bytes()
for ea,v,n in [(0x6056C78,0x60569E8,4),(0x60569EA,0xE302,2),
 (0x60569EE,0x0436,2),(0x60569F2,0xE02A,2),(0x6056A20,0x0314,2),
 (0x6056A5C,0x1FD4,2),(0x6056A60,0x730C,2),(0x6056A64,0x1F35,2),
 (0x6056B5C,0x907A,2),(0x6056B5E,0x304C,2),(0x6056B62,0xC907,2)]:
 o=ea-0x6054000;assert int.from_bytes(p[o:o+n],'big')==v,f'Native passage contract differs at {ea:08X}'
print('PASS original passage callback, visit-bit write, world/debris transform and adjusted-bit mask instructions')

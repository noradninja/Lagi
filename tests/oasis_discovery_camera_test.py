from pathlib import Path
import struct,subprocess,tempfile,sys
r=Path(__file__).resolve().parents[1];b=Path(sys.argv[1]).read_bytes();base=0x6054000
u=lambda a:struct.unpack_from('>I',b,a-base)[0]
assert u(0x605A2A4)==0x607272E
assert u(0x605A2A0)==0x608CFB4 and u(0x605A29C)==0x6099FD4
frames=[];p=0x608CFB4
while u(p):frames.append(struct.unpack_from('>9I',b,p-base));p+=36
assert [x[0] for x in frames]==[185,200]
assert all(x[3]==0 and x[7]==0 and x[4]==0xA0000 and x[8] in (0xA0000,0x78000) for x in frames)
raw=b[0x608CFB4-base:p-base+36]
fixture=r"""
#include <vector>
#include <cassert>
#include <cstdint>
using u32=uint32_t;using s32=int32_t;
struct sVec3_FP{int x,y,z;};struct s_scriptData2{u32 m0,m4,m8,mC,m10,m14,m18,m1C,m20;};
const unsigned char data[]={DATA};
struct sSaturnPtr{unsigned offset; sSaturnPtr operator+(unsigned n)const{return {offset+n};}void operator+=(unsigned n){offset+=n;}};
u32 readSaturnU32(sSaturnPtr p){u32 n=0;for(unsigned i=0;i<4;++i)n=n*256+data[p.offset+i];return n;}
struct sA5SandfallEntity{std::vector<s_scriptData2> lagiDiscoveryOrbitFrames;}actor;
sVec3_FP anchor{1,2,3};int calls=0;
void cutsceneTaskInitSub3(sA5SandfallEntity* p,std::vector<s_scriptData2>& f,s32 cb,sVec3_FP* target,u32 flags){assert(p==&actor&&&f==&actor.lagiDiscoveryOrbitFrames&&target==&anchor&&cb==0&&flags==0);assert(f.size()==3&&f[0].m0==185&&f[1].m0==200&&!f[2].m0);assert(f[0].m4==266944147&&f[0].m14==260978915&&f[1].m10==655360);++calls;}
// PRODUCTION
int main(){lagiStartOasisDiscoveryOrbitCamera(&actor,{0},0,&anchor,0);lagiStartOasisDiscoveryOrbitCamera(&actor,{0},0,&anchor,0);assert(calls==2&&actor.lagiDiscoveryOrbitFrames.size()==3);}
""".replace('DATA',','.join(map(str,raw))).replace('// PRODUCTION',(r/'src/integration/lagi_oasis_discovery_camera.inc').read_text())
with tempfile.TemporaryDirectory() as d:
 p=Path(d);(p/'test.cpp').write_text(fixture)
 subprocess.run(['cl','/nologo','/EHsc',str(p/'test.cpp'),'/Fe:'+str(p/'test.exe'),'/Fo:'+str(p/'test.obj')],check=True)
 subprocess.run([str(p/'test.exe')],check=True)
s=(r/'build-vita/azel_upstream/field_field_a5_a5_sandfallEntity.cpp').read_text()
assert 'lagiStartOasisDiscoveryOrbitCamera(pThis, gFLD_A5->getSaturnPtr(0x0608CFB4)' in s
assert 'a5CutsceneCameraInit((p_workArea)pThis' not in s
print('PASS original orbit dispatcher, 185/200-frame records, production decoder, owned storage, shared service and generated callback')

for ea,word in [(0x60723A8,0x52EA),(0x60723AE,0x03EE),(0x60723B2,0x323C),(0x60723B6,0x1E2A)]:
 assert struct.unpack_from('>H',b,ea-base)[0]==word
s=(r/'build-vita/azel_upstream/field_fieldCutsceneTask3.cpp').read_text()
assert s.count('pThis->m28 += pThis->m48;')==1
assert s.index('pThis->m28 += pThis->m48;')<s.index('pThis->m8[0] =',s.index('case 1:'))
print('PASS native per-tick radius integration restored before orbit position evaluation')

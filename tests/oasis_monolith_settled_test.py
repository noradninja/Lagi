from pathlib import Path
import subprocess,tempfile,sys
r=Path(__file__).resolve().parents[1]
f=r"""
#include <cassert>
#include <cstdint>
#include <array>
#include <cstdio>
using s32=int32_t;using u32=uint32_t;using p_workArea=void*;
struct fixedPoint{int v=0;fixedPoint(int n=0):v(n){}fixedPoint& operator+=(fixedPoint n){v+=n.v;return *this;}};
struct sVec3_FP:std::array<fixedPoint,3>{};
struct s_fileBundle{uint8_t raw[32]{};uint8_t* getRawBuffer(){return raw;}};
unsigned READ_BE_U32(uint8_t*){return 0x1234;}
struct sDebrisScatterParams{fixedPoint m8_spread,m28_rotVelY,m30_angleOffset,m38;unsigned m44_soundEffect;unsigned model,pose;sVec3_FP* m10_pPosition; sVec3_FP* m14_pRotation;s_fileBundle* m_pBundle;};
struct sA5SkyEntity{struct{s_fileBundle* m0_mainMemoryBundle;}m0_memoryArea;sVec3_FP mE8_position,mF8_rotation;int m104=0,m108=0,m114=0;bool m11C_flag=false;void* lagiDebris=nullptr;void(*m_DrawMethod)(sA5SkyEntity*)=nullptr;struct{void(*m18_drawFunction)(void*);}m48_model1;};
bool culled=false;int draws=0,pushes=0,pops=0,sounds=0,found=0;unsigned configs=0;bool reversed=false;sDebrisScatterParams captured;
void pushCurrentMatrix(){++pushes;}void popMatrix(){++pops;}void translateCurrentMatrix(sVec3_FP*){}void rotateCurrentMatrixZYX(sVec3_FP*){}
bool checkPositionVisibilityAgainstFarPlane(sVec3_FP*){return culled;}
int findSound(int n){assert(n==0x70);return found;}
void enqueuePlaySoundEffect(int id,int command,int value,int arg){assert(id==0x70&&command==5&&!value&&arg==2);++sounds;}
void initDebrisScatterConfig(sDebrisScatterParams* c,unsigned model,unsigned pose){*c={};c->model=model;c->pose=pose;}
int readMaxScalarFromBundleTree(uint8_t*,unsigned root){assert(root==0x1234);return 0x4567;}
void* createDebrisScatterTask(sA5SkyEntity* a,sDebrisScatterParams* c,bool reverse){++configs;captured=*c;reversed=reverse;return a;}
// PRODUCTION
int main(){
 s_fileBundle bundle;for(bool reverse:{false,true}){sA5SkyEntity actor;actor.m0_memoryArea.m0_mainMemoryBundle=&bundle;actor.m104=0x1234;actor.m108=-0x800;actor.mF8_rotation[1]=0x1000;
 lagiOasisMonolithSetupDebris(&actor,reverse);assert(captured.model==0x18&&captured.pose==0xB50&&captured.m8_spread.v==0x4567&&captured.m38.v==0xA000&&captured.m44_soundEffect==0x6F&&captured.m28_rotVelY.v==-0x400&&captured.m30_angleOffset.v==-0x8000);assert(captured.m10_pPosition==&actor.mE8_position&&captured.m14_pRotation==&actor.mF8_rotation&&captured.m_pBundle==&bundle&&reversed==reverse&&actor.lagiDebris==&actor&&actor.m_DrawMethod==lagiOasisMonolithSettledDraw&&actor.mF8_rotation[1].v==0x2234);
 }
 for(int initial=0;initial<=240;++initial)for(found=-1;found<=0;++found){sA5SkyEntity actor;actor.m114=initial;sounds=0;for(int tick=0;tick<242;++tick){culled=tick%2;lagiOasisMonolithSettledUpdate(&actor);assert(actor.m11C_flag==!culled);}assert(actor.m114==0&&sounds==(initial&&found>=0?1:0));}
 sA5SkyEntity actor;actor.m48_model1.m18_drawFunction=[](void*){++draws;};lagiOasisMonolithSettledDraw(&actor);assert(!draws);actor.m11C_flag=true;lagiOasisMonolithSettledDraw(&actor);assert(draws==1&&pushes==1&&pops==1);
 puts("PASS production Oasis settled/debris binding, reverse reload, sound countdown, culling and primary-only drawing");
}
""".replace('// PRODUCTION',(r/'src/integration/lagi_oasis_monolith_settled.inc').read_text())
with tempfile.TemporaryDirectory(prefix='lagi-oasis-settled-') as d:
 p=Path(d);(p/'test.cpp').write_text(f)
 subprocess.run(['cl','/nologo','/EHsc','/std:c++20',str(p/'test.cpp'),'/Fe:'+str(p/'test.exe'),'/Fo:'+str(p/'test.obj')],check=True)
 subprocess.run([str(p/'test.exe')],check=True)
s=(r/'build-vita/azel_upstream/field_field_a5_a5_skyEntity.cpp').read_text();assert 'lagiOasisMonolithSetupDebris(p,true);' in s and 'p->m_UpdateMethod=&lagiOasisMonolithSettledUpdate;' in s
b=Path(sys.argv[1]).read_bytes();base=0x6054000
for ea,value,n in [(0x605AF0A,0xE518,2),(0x605AF86,0xB50,2),(0x605AF9C,0xA000,4),(0x605AF62,0x0F25,2),(0x605AF60,0xE048,2),(0x605AF5E,0xE26F,2),(0x605AF7A,0x1E33,2),(0x605AFA8,0x605AD7A,4),(0x605AD14,0x605B1FC,4)]:
 assert int.from_bytes(b[ea-base:ea-base+n],'big')==value,f'Native settled contract changed at {ea:08X}'
print('PASS original saved-state debris/model/draw/update contracts')

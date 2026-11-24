from pathlib import Path
import subprocess,tempfile,sys
r=Path(__file__).resolve().parents[1]
f=(r/'tests/nanyd_motion_test.cpp').read_text().split('struct PositionBlock',1)[0]
f=f.replace('struct fixedPoint{','struct fixedPoint{friend fixedPoint operator+(fixedPoint a,fixedPoint b){return lagi::formationAdd(a.m_value,b.m_value);}')
f+=r"""
struct sA5SkyEntity{sVec3_FP mE8_position;};struct sVdp1Quad{};struct sParticlePoolManager{};
struct sParticleSpawnConfig{const std::vector<sVdp1Quad>* m8_pQuadData;};
struct sAnimRingArg{sVec3_FP m00_center,m0C_ring0,m18_ring1,m24_direction;int m30_lifetime=0,m34_mode=0;};
struct File{sSaturnPtr getSaturnPtr(u32 a){return {a};}}file;auto* gFLD_A5=&file;
struct s_fieldSpecificData_A5{sParticlePoolManager* m54_particlePool;}field;
struct Field{void* mC=&field;}fieldTask;Field* getFieldTaskPtr(){return &fieldTask;}
std::vector<sVdp1Quad> initVdp1Quad(sSaturnPtr p){assert(p.address==0x609A008);return {{}};}
unsigned randomCalls=0;u32 randomNumber(){++randomCalls;return 0x12345678;}
fixedPoint getCos(u32){return 0x10000;}fixedPoint getSin(u32){return 0;}
fixedPoint MTH_Mul(fixedPoint a,fixedPoint b){return lagi::formationMultiply(a.asS32(),b.asS32());}
std::vector<sAnimRingArg> spawned;void spawnAnimRingSubTask(sA5SkyEntity*,sAnimRingArg* arg){spawned.push_back(*arg);}
sVec3_FP burstPosition,burstVelocity;int sounds=0;
void spawnParticleProjected(sParticlePoolManager*,sParticleSpawnConfig* c,sVec3_FP* p,sVec3_FP* v){assert(c->m8_pQuadData);burstPosition=*p;burstVelocity=*v;}
void enqueuePlaySoundEffect(int id,int command,int value,int flags){assert(id==0x72&&command==1&&value==0x73&&!flags);++sounds;}
// EFFECTS
int main(){
 sA5SkyEntity actor;actor.mE8_position={0x10000,0x20000,-0x30000};
 lagiOasisMonolithBurst(&actor);assert(randomCalls==3&&sounds==1&&burstVelocity[1].asS32()==0xCC);
 spawned.clear();randomCalls=0;lagiOasisMonolithRing(&actor,true);assert(randomCalls==1&&spawned.size()==1);auto a=spawned[0];assert(a.m30_lifetime==12&&a.m34_mode==1&&a.m00_center[1].asS32()==0x5E000&&a.m18_ring1[1].asS32()==0xC000&&a.m24_direction[0].asS32()==0x17000&&a.m24_direction[1].asS32()==0xA000);
 spawned.clear();randomCalls=0;lagiOasisMonolithRing(&actor,false);assert(randomCalls==2&&spawned.size()==1);a=spawned[0];assert(a.m30_lifetime==3&&a.m34_mode==0&&a.m0C_ring0[1].asS32()==0&&a.m18_ring1[1].asS32()==0&&a.m0C_ring0[0].asS32()==0x15000&&a.m18_ring1[0].asS32()==0x1F000&&a.m24_direction[1].asS32()==0x1E000);
 spawned.clear();randomCalls=0;lagiOasisMonolithFinalRings(&actor);assert(randomCalls==24&&spawned.size()==8);for(const auto& ring:spawned){assert(ring.m34_mode==1&&ring.m00_center[1].asS32()==0&&ring.m18_ring1[1].asS32()==0&&ring.m30_lifetime==int((0x12345678&63)+30));assert(ring.m18_ring1[0].asS32()==0x10000+int((0x12345678+0x32000)&0x3FFFF));}
 puts("PASS production monolith effect descriptors: native small/radial targets, burst, eight final arcs, lifetimes and exact spawn RNG counts");
}
""".replace('// EFFECTS',(r/'src/integration/lagi_oasis_monolith_effects.inc').read_text())
with tempfile.TemporaryDirectory(prefix='lagi-oasis-effects-') as d:
 p=Path(d);(p/'test.cpp').write_text(f)
 subprocess.run(['cl','/nologo','/EHsc','/std:c++20','/I'+str(r/'include'),str(p/'test.cpp'),'/Fe:'+str(p/'test.exe'),'/Fo:'+str(p/'test.obj')],check=True)
 subprocess.run([str(p/'test.exe')],check=True)
b=Path(sys.argv[1]).read_bytes();base=0x6054000
for ea,value,n in [(0x605B9A2,0xCC,2),(0x605B9A8,0x26000,4),(0x605B9B8,0x609A008,4),(0x605BA86,0xE203,2),(0x605BAD8,0x1E000,4),(0x605BB4E,0xE30C,2),(0x605BB80,0xC000,4),(0x605BB88,0x17000,4),(0x605BD00,0x2000000,4),(0x605BD04,0x10000000,4)]:
 assert int.from_bytes(b[ea-base:ea-base+n],'big')==value,f'Original monolith effect operand changed at {ea:08X}'
print('PASS original monolith particle/arc operands and eight-spawn angle range')

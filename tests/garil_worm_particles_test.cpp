#include <cassert>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <vector>
struct sVec3_FP {int32_t m0_X=0,m4_Y=0,m8_Z=0;};
struct Part {sVec3_FP m4_worldPosition;};
struct sMonsterBody {std::vector<Part> m30_parts;};
struct sParticlePoolManager {};
struct sParticleSpawnConfig {sVec3_FP *m0_pPosition=nullptr,*m4_pVelocity=nullptr;const void* m8_pQuadData=nullptr;};
struct FieldData {sParticlePoolManager* m54_particlePool;};using s_fieldSpecificData_A5=FieldData;
struct Field {FieldData* mC;};
static sParticlePoolManager manager;static FieldData data{&manager};static Field field{&data};
Field* getFieldTaskPtr(){return &field;}
struct Overlay {uintptr_t getSaturnPtr(uintptr_t ea){return ea;}};
static Overlay overlay;static Overlay* gFLD_A5=&overlay;
const void* a5GetOrParseQuadList(uintptr_t ea){return reinterpret_cast<void*>(ea);}
int centeredRandom(int range){assert(range==0xFFFF);static int n=0;return ++n;}
struct Emission {sVec3_FP position,velocity;uintptr_t quads;int cycles;};
static std::vector<Emission> emissions;
void spawnParticleProjected(sParticlePoolManager* pool,sParticleSpawnConfig* desc,sVec3_FP* p,sVec3_FP* v) {
 assert(pool==&manager);emissions.push_back({*p,*v,reinterpret_cast<uintptr_t>(desc->m8_pQuadData),0});
}
int lagiSpawnFieldParticleFollowingXZ(sParticlePoolManager* pool,const sParticleSpawnConfig& desc,const std::shared_ptr<sVec3_FP>& position,int cycles) {
 assert(pool==&manager);emissions.push_back({*position,*desc.m4_pVelocity,reinterpret_cast<uintptr_t>(desc.m8_pQuadData),cycles});return 1;
}
// PRODUCTION_PRODUCERS
int32_t fixedPoint(int32_t value){return value;}
int32_t MTH_Mul(int32_t a,int32_t b){return static_cast<int32_t>((int64_t(a)*b)>>16);}
struct Actor {std::shared_ptr<sMonsterBody> body;int partCount;std::vector<int32_t> oldPartY;std::vector<uint8_t> partEffects;};
void emit(Actor* self) {auto& b=*self->body;
// PRODUCTION_LOOP
}
int main() {
 sVec3_FP p{100,200,300};lagiSpawnA5WormSegmentSand(&p);
 assert(emissions.size()==4);
 for(int i=0;i<4;++i) {
  const auto& e=emissions[i];assert(e.position.m0_X==101+i*2&&e.position.m8_Z==302+i*2&&e.position.m4_Y==200);
  assert(e.quads==0x0608C6E4&&e.cycles==0);
  assert(e.velocity.m0_X==0&&e.velocity.m4_Y==0&&e.velocity.m8_Z==0);
 }
 auto body=std::make_shared<sMonsterBody>();body->m30_parts.resize(8);
 body->m30_parts[3].m4_worldPosition=p;lagiSpawnA5WormSegmentDebris(body,3);
 assert(emissions.back().quads==0x0608C5E8&&emissions.back().cycles==2);
 assert(emissions.back().position.m4_Y==200&&emissions.back().velocity.m4_Y==0);
 // Every segment crossing emits; odd buried segments emit the linked effect.
 Actor actor{body,8,std::vector<int32_t>(8,0x10000),std::vector<uint8_t>(8,1)};
 for(auto& part:body->m30_parts)part.m4_worldPosition={0,-0x10000,0};
 emissions.clear();emit(&actor);assert(emissions.size()==8*4+4);
 for(int i=0;i<8;++i)assert(actor.partEffects[i]==(i&1));
 // Remaining below ground neither repeats the effect nor resets its latch.
 actor.oldPartY.assign(8,-0x10000);emissions.clear();emit(&actor);assert(emissions.empty());
 // Crossing back above ground resets the latch on every segment, no early break.
 for(auto& part:body->m30_parts)part.m4_worldPosition.m4_Y=0x10000;
 emit(&actor);assert(emissions.size()==32);
 for(auto flag:actor.partEffects)assert(flag==0);
 std::puts("PASS native worm producer bindings, zero velocities, four sand sprites, two cycles, all-segment story crossings and once-only buried latches");
}

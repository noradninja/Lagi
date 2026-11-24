#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unordered_map>
#include <memory>
#include <new>
using s32=int32_t;using u32=uintptr_t;using u16=uint16_t;
struct sVec3_FP {int m0_X=0,m4_Y=0,m8_Z=0;};
using Vec=sVec3_FP;
struct Quad {};
struct sParticleSlot {
 Vec m0_position,mC_velocity;int m18_velocityScaleX=0,m1C_velocityScaleY=0;
 void* m20_heapData=nullptr;int(*m24_updateFunc)(sParticleSlot*)=nullptr;
 void(*m28_drawFunc)(sParticleSlot*)=nullptr;Quad m2C_animQuad;
};
struct sParticlePoolManager {
 struct {uintptr_t m4_characterArea=0x25C00000;}m0_memoryArea;
 sParticleSlot *m8_slotsBase,*mC_currentSlot;
 int m10_currentIndex=0,m14_maxParticles=3,m18_activeCount=0,m1C_peakActiveCount=0;
};
struct sParticleSpawnConfig {
 Vec *m0_pPosition,*m4_pVelocity;void* m8_pQuadData=nullptr;
 int mC_velocityScaleX=0,m10_velocityScaleY=0;
 int(*m14_updateFunc)(sParticleSlot*);int m18_heapSize;void* m1C_heapData;
};
static std::unordered_map<void*,sParticlePoolManager*> allocations;
static unsigned frees=0;static bool failAllocation=false;
void* allocateHeapForTask(sParticlePoolManager* owner,unsigned bytes) {
 if(failAllocation)return nullptr;
 auto p=std::malloc(bytes);assert(p);allocations[p]=owner;return p;
}
void freeHeapForTask(sParticlePoolManager* owner,void* p) {
 assert(allocations.at(p)==owner);allocations.erase(p);++frees;std::free(p);
}
struct SubField {int m370_fieldDebuggerWho=0,m37E_debugMenuStatus2_a=0,m369=0;};
struct Field {SubField* m8_pSubFieldData;};
static SubField sub;static Field field{&sub};
Field* getFieldTaskPtr(){return &field;}
struct {int m10_palette=0;}vdp2PrintStatus;
void vdp2DebugPrintSetPosition(int,int){}
void vdp2PrintfSmallFont(const char*,int,int){}
void particleInitSub(Quad*,u16,void*){}
void particleDrawSimple(sParticleSlot*){}
void particleDrawBillboard(sParticleSlot*){}
s32 spawnParticleInPool(sParticlePoolManager*,sParticleSpawnConfig*,s32);
static int animationEvent=0;
int sGunShotTask_UpdateSub4(Quad*){return animationEvent;}
// PRODUCTION_FOLLOW
// PRODUCTION_POOL_UPDATE
// PRODUCTION_POOL_SPAWN
int pending(sParticleSlot*){return 0;}
int finished(sParticleSlot*){return 2;}
int main() {
 sParticleSlot slots[3];sParticlePoolManager pool{ {},slots,slots };
 Vec position{1,2,3},velocity{4,5,6};int payload=0x12345678;
 sParticleSpawnConfig desc{&position,&velocity,nullptr,0,0,finished,sizeof(payload),&payload};
 // A long-running pool must not retain completed payloads until field teardown.
 for(int cycle=0;cycle<10000;++cycle) {
  for(int i=0;i<3;++i)assert(spawnParticleInPool(&pool,&desc,i&1)==1);
  assert(pool.m18_activeCount==3&&allocations.size()==3);
  assert(spawnParticleInPool(&pool,&desc,0)==0);
  assert(allocations.size()==3);
  for(auto& slot:slots)assert(*static_cast<int*>(slot.m20_heapData)==payload);
  particlePoolUpdate(&pool);
  assert(pool.m18_activeCount==0&&allocations.empty());
  for(auto& slot:slots)assert(!slot.m20_heapData&&!slot.m28_drawFunc);
  particlePoolUpdate(&pool);assert(allocations.empty());
 }
 assert(frees==30000);
 // A live particle retains its payload; a payload-free finished slot is valid.
 desc.m14_updateFunc=pending;assert(spawnParticleInPool(&pool,&desc,0));
 particlePoolUpdate(&pool);assert(allocations.size()==1&&pool.m18_activeCount==1);
 pool.mC_currentSlot->m24_updateFunc=finished;
 desc.m18_heapSize=0;desc.m1C_heapData=nullptr;desc.m14_updateFunc=finished;
 assert(spawnParticleInPool(&pool,&desc,0));
 particlePoolUpdate(&pool);assert(allocations.empty()&&pool.m18_activeCount==0&&frees==30001);
 // Allocation failure must not publish a slot or alter the active count.
 desc.m18_heapSize=4;failAllocation=true;
 assert(!spawnParticleInPool(&pool,&desc,0));assert(pool.m18_activeCount==0);
 assert(allocations.empty());
 // Follow the original live X/Z plus offsets, with floor Y and two cycle events.
 failAllocation=false;
 auto owner=std::make_shared<Vec>(Vec{100,200,300});std::weak_ptr<Vec> weak=owner;
 velocity={7,8,9};desc.m4_pVelocity=&velocity;
 const long refs=owner.use_count();failAllocation=true;
 assert(!lagiSpawnFieldParticleFollowingXZ(&pool,desc,owner,2));
 assert(owner.use_count()==refs);failAllocation=false;
 assert(lagiSpawnFieldParticleFollowingXZ(&pool,desc,owner,2));
 auto* slot=pool.mC_currentSlot;
 owner->m0_X=1000;owner->m8_Z=3000;
 owner.reset();assert(!weak.expired());
 animationEvent=0;particlePoolUpdate(&pool);
 assert(slot->m0_position.m0_X==1007&&slot->m0_position.m4_Y==8&&slot->m0_position.m8_Z==3009);
 animationEvent=1;particlePoolUpdate(&pool);assert(!weak.expired());
 animationEvent=2;particlePoolUpdate(&pool);assert(!weak.expired()&&pool.m18_activeCount==1);
 animationEvent=0;particlePoolUpdate(&pool);assert(!weak.expired());
 animationEvent=2;particlePoolUpdate(&pool);
 assert(weak.expired()&&allocations.empty()&&pool.m18_activeCount==0);
 // Pool teardown releases remaining anchors regardless of actor/pool delete order.
 owner=std::make_shared<Vec>();weak=owner;
 assert(lagiSpawnFieldParticleFollowingXZ(&pool,desc,owner,2));owner.reset();
 lagiDeleteFieldParticlePayloads(&pool);
 assert(weak.expired()&&allocations.empty()&&pool.m18_activeCount==0);
 lagiDeleteFieldParticlePayloads(&pool);assert(allocations.empty());
 std::puts("PASS live X/Z following, two-cycle completion, allocation failure and owner/pool teardown");
 std::puts("PASS shared particle lifetime: 30000 retire/reuse cycles, live/null payloads, full pool and allocation failure");
}

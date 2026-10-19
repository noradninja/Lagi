#include <array>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
using s32=int32_t;
using u32=uint32_t;
using fixedPoint=int32_t;
using sVec3_FP=std::array<int32_t,3>;
struct sLCSTarget { int m18_diableFlags=0; };
struct Data { sVec3_FP m4_position{0,123,0}; };
static int targetUpdates,smoke,nearest,height,visibility;
static bool invisible;
static std::array<int32_t,2> endpoint;
static int32_t readSaturnS32(int a) { return endpoint.at(a/4); }
static void updateLCSTarget(sLCSTarget*) { ++targetUpdates; }
static void A3_0_Obj3Update1Sub2(sVec3_FP&) { ++nearest; }
static void A3_0_Obj3Update2Sub0(sVec3_FP& p,fixedPoint y) { ++height;p[1]=y; }
static bool checkPositionVisibilityAgainstFarPlane(sVec3_FP*) { ++visibility;return invisible; }
struct s_A3_0_Obj3;
static void createSmokePufTask(s_A3_0_Obj3*,sVec3_FP*,sVec3_FP*) { ++smoke; }
struct s_A3_0_Obj3 {
    Data data;
    Data* m8=&data;
    int mC=0;
    sVec3_FP m10_position{},m1C_velocity{};
    sLCSTarget m38_LCSTarget;
    int8_t m6C=0,m6E_visible=0;
    bool finished=false;
    s_A3_0_Obj3* getTask() { return this; }
    void markFinished() { finished=true; }
#include "lagi_a3_shootable_cart_update.inc"
};
using p_workArea=s_A3_0_Obj3*;
static struct {
    int bit=-1;
    void setBit(int b) { bit=b; }
} mainGameState;
#include "cart_callback.inc"
template<class T> struct s_workAreaTemplate {
    // Deliberately nonempty C++ header: a raw task-to-position cast is wrong.
    std::array<int32_t,9> header{};
    bool finished=false;
    struct TypedTaskDefinition { void(*init)(T*);void(*update)(T*);void(*draw)(T*);void(*destroy)(T*); };
    s_workAreaTemplate* getTask() { return this; }
    void markFinished() { finished=true; }
};
struct sParticleSlot {};
static s32 particleUpdateMoving(sParticleSlot*) { return 0; }
struct sParticleSpawnConfig {
    sVec3_FP* m0_pPosition; sVec3_FP* m4_pVelocity; int* m8_pQuadData;
    s32 mC_velocityScaleX,m10_velocityScaleY;
    s32(*m14_updateFunc)(sParticleSlot*);int m18_heapSize;void* m1C_heapData;
};
struct sParticlePoolManager {};
static sParticlePoolManager pool;
static struct { sParticlePoolManager* m168_particlePool=&pool; } fieldData;
static auto* getFieldSpecificData_A3() { return &fieldData; }
static struct { int m_mineCartDustQuad; } field;
static auto* gFLD_A3=&field;
static int rngCalls,particles;
static u32 randomNumber() { ++rngCalls;return 0x812; }
static sVec3_FP* expectedPosition;
static sVec3_FP expectedVelocity;
static void spawnParticleInPool(sParticlePoolManager* p,sParticleSpawnConfig* c,int mode) {
    assert(p==&pool && mode==1 && c->m0_pPosition==expectedPosition);
    assert(*c->m4_pVelocity==expectedVelocity);
    assert(c->m8_pQuadData==&field.m_mineCartDustQuad && c->mC_velocityScaleX==0x10000);
    assert(c->m10_velocityScaleY==int32_t(0xfffb0000) && c->m14_updateFunc==particleUpdateMoving);
    assert(c->m18_heapSize==0 && c->m1C_heapData==nullptr);++particles;
}
#include "cart_smoke.inc"
static void reset() { targetUpdates=smoke=nearest=height=visibility=0;invisible=false;endpoint={0,0}; }
static void tick(s_A3_0_Obj3& cart) { s_A3_0_Obj3::Update2(&cart); }
int main()
{
    reset();
    s_A3_0_Obj3 idle;
    tick(idle);assert(idle.m6C==0 && !idle.finished && nearest==0 && smoke==0);
    assert(targetUpdates==1 && height==1 && visibility==1 && idle.m10_position[1]==123);
    invisible=true;tick(idle);assert(idle.m6E_visible==0);
    A3_0_Obj3_LCSCallback(&idle,nullptr);
    assert(idle.m6C==1 && (idle.m38_LCSTarget.m18_diableFlags&1));
    assert(mainGameState.bit==0x49c);

    // Both direction signs and both axes reach the endpoint, emit once,
    // continue in state 2, then retire through the task's normal owner.
    for(int sign: {-1,1}) {
        reset();s_A3_0_Obj3 cart;
        endpoint={sign*0x40000,sign*0x40000};
        cart.m1C_velocity={sign*0x4000,0,sign*0x4000};
        A3_0_Obj3_LCSCallback(&cart,nullptr);
        for(int n=0;n<40 && !cart.finished;++n) tick(cart);
        assert(cart.finished && cart.m6C==2 && smoke==1);
        assert(targetUpdates==height && height==visibility && nearest>0);
    }
    // Native proximity threshold is strict, checked after movement.
    reset();s_A3_0_Obj3 boundary;boundary.m6C=1;
    boundary.m10_position={-0x10000,0,-0x10000};tick(boundary);
    assert(smoke==0 && boundary.m6C==1);
    boundary.m10_position={-0xffff,0,-0xffff};tick(boundary);
    assert(smoke==1 && boundary.m6C==2);
    // Do not replace the original state-2 X-vs-Z-endpoint read with Z.
    reset();s_A3_0_Obj3 authored;authored.m6C=2;
    authored.m1C_velocity={1,0,1};authored.m10_position={2,0,-100};endpoint={1,1};
    tick(authored);assert(authored.finished && nearest==0);
    reset();s_A3_0_Obj3 opposite;opposite.m6C=2;
    opposite.m1C_velocity={-1,0,-1};opposite.m10_position={0,0,100};endpoint={1,1};
    tick(opposite);assert(opposite.finished);
    reset();s_A3_0_Obj3 moving;moving.m6C=2;moving.m1C_velocity={2,0,3};
    endpoint={100,100};tick(moving);
    assert(!moving.finished && moving.m10_position[0]==2 && moving.m10_position[2]==3 && nearest==1);
    reset();s_A3_0_Obj3 unknown;unknown.m6C=99;tick(unknown);
    assert(!unknown.finished && height==1 && visibility==1);
    sSmokePufTask emitter;
    sVec3_FP source={123,456,789},velocity={10,20,30};
    emitter.m0_position={111,222,333};emitter.mC_pPositionRef=&source;
    emitter.m10_pVelocityRef=&velocity;emitter.m14_countdown=32;
    for(int n=31;n>=0;--n) {
        expectedPosition=n%2 ? &source : &emitter.m0_position;
        expectedVelocity=n%2 ? sVec3_FP{-996,0x812-0x333,-976} : sVec3_FP{18,0x812-0x333,18};
        sSmokePufTask::Update(&emitter);
    }
    assert(particles==32 && rngCalls==96 && !emitter.finished);
    sSmokePufTask::Update(&emitter);assert(emitter.finished && particles==32 && rngCalls==96);
    fieldData.m168_particlePool=nullptr;emitter.m14_countdown=2;sSmokePufTask::Update(&emitter);
    assert(particles==32 && rngCalls==99);
    puts("PASS native shootable-cart callback, persistence bit, all states, signed motion, strict proximity, smoke, retirement and visibility");
}

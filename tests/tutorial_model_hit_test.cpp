#include <cassert>
#include <cstdint>
#include <cstdio>
using u8=std::uint8_t;
struct sVec3_FP {int x{},y{},z{};};
struct sBattleTargetable {sVec3_FP* m4_pPosition{};sVec3_FP* mC{};};
struct Engine {sVec3_FP m1A0_battleAutoScrollDelta;};
struct Overlay {Engine* m4_battleEngine;};
struct Manager {Overlay* m10_battleOverlay;};
static Manager* gBattleManager;
static unsigned calls;
static sVec3_FP* seenPosition;
static sVec3_FP* seenVelocity;
static int seenScale;
static u8 seenMode;
static void createExplosionAtPosition(sVec3_FP* p,sVec3_FP* v,int scale,u8 mode) {
    ++calls;seenPosition=p;seenVelocity=v;seenScale=scale;seenMode=mode;
}
#include "../src/integration/lagi_tutorial_model_hit.inc"
int main() {
    Engine engines[2]{};Overlay overlay{&engines[0]};Manager manager{&overlay};
    gBattleManager=&manager;
    sVec3_FP current{1,2,3},impact{4,5,6};
    sBattleTargetable target{&current,&impact};
    // No-op must not query engine state or submit any particle.
    gBattleManager=nullptr;
    assert(lagiDispatchTutorialModelHit(0x0609f440,target)&&calls==0);
    assert(!lagiDispatchTutorialModelHit(0,target)&&calls==0);
    gBattleManager=&manager;
    const std::uint32_t callbacks[]={0x0609f444,0x0609f468,0x0609f48c};
    for(unsigned i=0;i<3;++i) for(unsigned e=0;e<2;++e) {
        overlay.m4_battleEngine=&engines[e];
        auto before=calls;
        assert(lagiDispatchTutorialModelHit(callbacks[i],target));
        assert(calls==before+1&&seenPosition==&impact&&seenPosition!=&current);
        assert(seenVelocity==&engines[e].m1A0_battleAutoScrollDelta);
        assert(seenScale==0x20000&&seenMode==(i==0?1:0));
    }
    // Adjacent/foreign addresses must never become executable pointers or
    // silently select a default effect. Full low-word range covers all T0 IDs.
    auto before=calls;
    for(unsigned low=0;low<65536;++low) {
        const auto ea=0x06090000u|low;
        const bool known=low==0xf440||low==0xf444||low==0xf468||low==0xf48c;
        assert(lagiDispatchTutorialModelHit(ea,target)==known);
    }
    assert(calls==before+3);
    std::puts("PASS: native T0 hit callback identities, live velocity and impact pointers, scale/mode, no-op and 65536 address boundaries; particle submission mocked, not hardware");
}

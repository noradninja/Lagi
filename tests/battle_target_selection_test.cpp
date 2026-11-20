#include <array>
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
using s32=std::int32_t;using u32=std::uint32_t;using s16=std::int16_t;using u8=std::uint8_t;
using fixedPoint=s32;using sVec3_FP=std::array<s32,3>;
struct sBattleTargetable {u32 m50_flags{};sVec3_FP *m4_pPosition{},m10_position{},m40{};s16 m5A{};};
struct s_battleEnemy {s32 m0_isActive=-1;sBattleTargetable* m4_targetable{};s32 m8_distanceToDragonSquare=0x7FFFFFFF;};
struct s_battleEngine {
    struct {bool m80=false,m1=false,m2_needToSortEnemiesByDistanceFromDragon=true;} m188_flags;
    std::array<s_battleEnemy,128> m49C_enemies{};
    std::array<s32,2> m35C_cameraAltitudeMinMax{100,-100};
    s32 m498_numEnemies{};u8 m22C_dragonCurrentQuadrant{};
};
struct sBattleOverlayTask_C {
    std::array<s_battleEnemy*,128> m0_enemyTargetables{};
    s32 m200_cameraMinAltitude{},m204_cameraMaxAltitude{},m20A_numSelectableEnemies{};
};
struct Dragon {sVec3_FP m8_position{};};
struct Debug {std::array<u8,32> mFlags{};};
struct Overlay {s_battleEngine* m4_battleEngine{};sBattleOverlayTask_C* mC_targetSystem{};Debug* m10_inBattleDebug{};Dragon* m18_dragon{};};
struct Manager {Overlay* m10_battleOverlay{};} manager,*gBattleManager=&manager;
static std::array<s16,4> viewport{-100,100,100,-100}; // left, top, right, bottom
static void getVdp1ClippingPlanes(fixedPoint& near,fixedPoint& far){near=1;far=1000;}
static void getVdp1ProjectionParams(s16* x,s16* y){*x=100;*y=100;}
static s32 setDividend(s32 scale,s32 v,s32 z){assert(z);return scale*v/z;}
static void getVdp1ScreenResolution(s16 (&out)[4]){std::copy(viewport.begin(),viewport.end(),out);}
static void battleTargetable_updatePosition(sBattleTargetable* t){t->m10_position=*t->m4_pPosition;}
static s32 distanceSquareBetween2Points(const sVec3_FP& a,const sVec3_FP& b){
    s32 out=0;for(unsigned i=0;i<3;++i){const s32 v=a[i]-b[i];out+=v*v;}return out;
}
#include "battle_target_selection_source.inc"
int main(){
    s_battleEngine engine;sBattleOverlayTask_C targets;Debug debug;Dragon dragon;
    Overlay overlay{&engine,&targets,&debug,&dragon};manager.m10_battleOverlay=&overlay;
    sVec3_FP position{0,0,100};sBattleTargetable target;target.m4_pPosition=&position;
    targets.m200_cameraMinAltitude=100;targets.m204_cameraMaxAltitude=-100;
    target.m10_position=position;
    // Normal native mode deliberately bypasses debug projection; it does not
    // bypass authored quadrant masks, disabled/retired states or laser-only bits.
    for(unsigned mask=0;mask<16;++mask)for(unsigned q=0;q<4;++q)
    for(unsigned flags=0;flags<16;++flags)for(unsigned laser=0;laser<2;++laser){
        engine=s_battleEngine{};targets=sBattleOverlayTask_C{};
        for(unsigned i=0;i<128;++i)targets.m0_enemyTargetables[i]=&engine.m49C_enemies[i];
        target.m50_flags=(mask<<28)|((flags&1)?1:0)|((flags&2)?2:0)|
            ((flags&4)?0x100000:0)|((flags&8)?0x40000:0)|0x20000;
        target.m5A=99;target.m10_position=position;
        engine.m22C_dragonCurrentQuadrant=q;engine.m188_flags.m1=laser!=0;
        engine.m498_numEnemies=1;engine.m49C_enemies[0]={0,&target,0};
        sBattleOverlayTask_C_Update(&targets);
        const bool eligible=(mask&(8>>q)) && !(flags&1) && !(flags&4) &&
            !(flags&8) && (laser || !(flags&2));
        assert(targets.m20A_numSelectableEnemies==int(eligible));
        assert(engine.m498_numEnemies==((flags&8) && !(flags&1)?0:1));
        if(!(flags&8))assert(target.m5A==int(eligible));
        if(eligible){assert(targets.m0_enemyTargetables[0]->m4_targetable==&target);
            assert(!laser || !(target.m50_flags&0x20000));}
        assert(!engine.m188_flags.m1);
    }
    target.m50_flags=0;target.m10_position={0,0,100};
    targets.m200_cameraMinAltitude=100;targets.m204_cameraMaxAltitude=-100;
    debug.mFlags[0x1F]=1;
    for(s32 x=-101;x<=101;++x)for(s32 y=-101;y<=101;++y){
        position={x,y,100};
        assert(sBattleOverlayTask_C_IsTargetableForGun(&target)==
            (x>=-100 && x<=100 && y>=-100 && y<=100));
    }
    for(s32 z:{0,1,1000,1001}){
        position={0,0,z};assert(sBattleOverlayTask_C_IsTargetableForGun(&target)==(z>=1 && z<=1000));
    }
    debug.mFlags[0x1F]=0;position={500,500,-100};
    assert(sBattleOverlayTask_C_IsTargetableForGun(&target));
    // Repeated quadrant changes clear stale selectable indices and preserve
    // distance order without treating an old pointer-array slot as eligibility.
    std::array<sVec3_FP,4> positions{{{0,0,40},{0,0,30},{0,0,20},{0,0,10}}};
    std::array<sBattleTargetable,4> mixed;
    constexpr std::array<u32,4> masks{0xC0000000,0x60000000,0x30000000,0x90000000};
    engine=s_battleEngine{};targets=sBattleOverlayTask_C{};
    for(unsigned i=0;i<128;++i)targets.m0_enemyTargetables[i]=&engine.m49C_enemies[i];
    for(unsigned i=0;i<4;++i){
        mixed[i].m4_pPosition=&positions[i];mixed[i].m50_flags=masks[i]|0x200000;
        engine.m49C_enemies[i]={0,&mixed[i],0};
    }
    for(unsigned q:{0,1,2,3,0,3,2,1}){
        engine.m22C_dragonCurrentQuadrant=q;
        engine.m188_flags.m2_needToSortEnemiesByDistanceFromDragon=true;
        sBattleOverlayTask_C_Update(&targets);
        assert(targets.m20A_numSelectableEnemies==2);
        unsigned nextIndex=1;
        // Target3 is closest, target0 farthest; pointer order from the prior
        // quadrant remains in place until the actual production sort runs.
        for(int i=3;i>=0;--i){
            if(masks[i]&(0x80000000u>>q)){
                assert(mixed[i].m5A==int(nextIndex));
                assert(targets.m0_enemyTargetables[nextIndex-1]->m4_targetable==&mixed[i]);
                ++nextIndex;
            }else assert(mixed[i].m5A==0);
        }
        for(unsigned i=0;i<4;++i){
            // The selection system owns m5A, not the gun engine's weak-label
            // latch. Do not conflate the two by clearing it in this adapter.
            assert(mixed[i].m50_flags&0x200000);
        }
    }
    puts("PASS production shared target selection: 2048 quadrant/state cases, native debug viewport boundaries and repeated quadrant changes; device visibility pending");
}

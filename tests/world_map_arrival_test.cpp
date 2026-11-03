#include <cassert>
#include <cstdint>
#include <cstdio>
#include <vector>
#include "lagi/world_map_arrival.h"
using s16=std::int16_t;using u32=std::uint32_t;using s32=std::int32_t;
struct s_workArea {} owner;
struct sVec3_FP {int value[3]{};};
struct sSaturnPtr {unsigned ea;};
struct Common {sSaturnPtr getSaturnPtr(unsigned ea){return {ea};}} common,*gCommonFile=&common;
sSaturnPtr readSaturnEA(sSaturnPtr p){assert(p.ea==0x206AC4+3*20);return {0x210000};}
s16 readSaturnS16(sSaturnPtr p){assert(p.ea==0x210000);return 3;}
struct sAnimationData {unsigned m4_numFrames=18;} animation;
struct s_3dModel {unsigned m16_previousAnimationFrame=0;sAnimationData* m30_pCurrentAnimation=nullptr;};
struct s3DModelAnimData {};
std::vector<int> calls;
bool animationPresent=true;
struct Bundle {sAnimationData* getAnimation(unsigned slot){assert(slot==0x60);calls.push_back(1);return animationPresent?&animation:nullptr;}} bundle;
const unsigned short animationOffsets[]={0x60};
struct Dragon {
    Bundle* m0_pDragonModelBundle=&bundle;
    const unsigned short* m20_dragonAnimOffsets=animationOffsets;
    unsigned m24_dragonAnimCount=1;
    int mC_dragonType=3;
    unsigned m14_modelIndex=0x20,m18_shadowModelIndex=0x24;
    s_3dModel m28_dragon3dModel;
    s3DModelAnimData m78_animData;
} dragon,*gDragonState=&dragon;
void loadDragon(s_workArea* p){assert(p==&owner);calls.push_back(0);gDragonState=&dragon;}
void setupDragonForTown(s_3dModel* p){assert(p==&dragon.m28_dragon3dModel);calls.push_back(2);}
unsigned setupModelAnimation(s_3dModel* p,sAnimationData* a){
    assert(p==&dragon.m28_dragon3dModel&&a==&animation);calls.push_back(3);p->m30_pCurrentAnimation=a;return 1;
}
void dragonMenuDragonInitSub3(s3DModelAnimData* p){assert(p==&dragon.m78_animData);calls.push_back(4);}
void stepAnimation(s_3dModel* p){assert(p==&dragon.m28_dragon3dModel);calls.push_back(5);}
void updateAnimationMatrices(s3DModelAnimData* a,s_3dModel* m){
    assert(a==&dragon.m78_animData&&m==&dragon.m28_dragon3dModel);calls.push_back(6);
}
namespace lagi::worldmap {
struct MapDisplayInput {sVec3_FP arrivalPosition,arrivalRotation;bool arrivalReady=true;};
}
lagi::worldmap::MapDisplayInput input;
void submitModelAndShadowModelToRendering(s_3dModel* model,unsigned main,unsigned shadow,
                                         sVec3_FP* position,sVec3_FP* rotation,int height){
    assert(model==&dragon.m28_dragon3dModel&&main==0x20&&shadow==0x24);
    assert(position==&input.arrivalPosition&&rotation==&input.arrivalRotation&&height==0);
    calls.push_back(7);
}
#include "production.inc"
int main(){
    using namespace lagi::worldmap;
    MapArrivalAnimation state;
    animationPresent=false;assert(!initialize_map_arrival_animation(state,&owner));
    assert(!state.ready);animationPresent=true;calls.clear();
    assert(initialize_map_arrival_animation(state,&owner)&&state.ready);
    assert((calls==std::vector<int>{1,2,3,4})); // resident dragon is not reloaded
    calls.clear();update_map_arrival_animation(state);
    assert(state.curveFrame==0&&(calls==std::vector<int>{5,6}));
    dragon.m28_dragon3dModel.m16_previousAnimationFrame=17;
    for(int expected:{1,2,0}){update_map_arrival_animation(state);assert(state.curveFrame==expected);}
    calls.clear();draw_map_arrival(state,input,0x64001);assert(calls.empty());
    draw_map_arrival(state,input,0x64000);assert((calls==std::vector<int>{7}));
    calls.clear();input.arrivalReady=false;draw_map_arrival(state,input,0);assert(calls.empty());
    input.arrivalReady=true;gDragonState=nullptr;
    assert(initialize_map_arrival_animation(state,&owner));
    assert((calls==std::vector<int>{0,1,2,3,4}));
    calls.clear();state.ready=false;update_map_arrival_animation(state);draw_map_arrival(state,input,0);
    assert(calls.empty());
    puts("PASS production WORLD arrival resident/load ownership, animation selection/order, curve cycle, native model/shadow draw and camera-height gate; not a hardware result");
}

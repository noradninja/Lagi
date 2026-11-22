#include <array>
#include <vector>
#include <cstdint>
#include <cassert>
#include <cstdio>
using u8=std::uint8_t;using u32=std::uint32_t;using s32=std::int32_t;
struct sVec3_FP:std::array<s32,3>{};
struct s_workArea{};struct s_workAreaCopy:s_workArea{};using p_workArea=s_workArea*;
struct sSaturnPtr{u32 address;};
struct File{sSaturnPtr getSaturnPtr(u32 x){return {x};}}file;
auto* g_BTL_A5=&file;
struct Target{sVec3_FP* m8;};
struct Model{Target* m54_targetables;unsigned m52_targetableCount;};
struct Formation{u8 mF0_flag=0;u8 mF1_pad5[3]{};};
struct sBTL_A5_SingleEnemy:s_workAreaCopy{
    std::vector<Model> m2C_models;
    int m1BC_isLead2{};u8 m28_state{};u32 m14_flags{};
    Formation* m158_formation;
    struct{uint8_t* activeMask=nullptr;uint8_t* reactionMask=nullptr;}m_lagiGroup;
};
struct Engine{sVec3_FP m1A0_battleAutoScrollDelta;};
struct Overlay{Engine* m4_battleEngine;};struct Manager{Overlay* m10_battleOverlay;};
Engine engine;Overlay overlay{&engine};Manager manager{&overlay};auto* gBattleManager=&manager;
struct Call{bool charge;s_workAreaCopy* parent;const sVec3_FP* sourcePtr;const u8* state;
    u8 active;sVec3_FP source,velocity;sSaturnPtr quad;u8 slot;int damage,flags;};
std::vector<Call> calls;std::vector<int> sounds;
p_workArea lagiCreateNativeBattleChargeQuad(s_workAreaCopy* parent,const sVec3_FP* p,
    const u8* state,u8 active,sSaturnPtr q,u8 slot){
    assert(*state==1);calls.push_back({true,parent,p,state,active,{}, {},q,slot,0,0});return nullptr;
}
p_workArea lagiCreateNativeHomingBattleProjectile(s_workAreaCopy* parent,
    const sVec3_FP& source,const sVec3_FP& velocity,sSaturnPtr q,u8 slot,int damage,int flags){
    calls.push_back({false,parent,nullptr,nullptr,0,source,velocity,q,slot,damage,flags});return nullptr;
}
unsigned randomCalls{},animationSteps{},positionUpdates{};
u32 randomNumber(){++randomCalls;return 5;}
s32 performModulo2(u32 divisor,u32 value){assert(divisor==3);return value%divisor;}
void stepAnimation(Model*){++animationSteps;}
void BTL_A5_singleEnemy_updatePositionFromFormation(sBTL_A5_SingleEnemy*){++positionUpdates;}
void playSystemSoundEffect(int n){sounds.push_back(n);}
// PRODUCTION_PRODUCERS
static void updateAttackStages(sBTL_A5_SingleEnemy* pThis)
{
    switch(pThis->m28_state){
// PRODUCTION_STAGES
    default:break;
    }
}
int main()
{
    for(bool lead:{false,true}){
        Formation formation{};sBTL_A5_SingleEnemy actor;
        sVec3_FP world[3]{{{100,200,300}},{{400,500,600}},{{700,800,900}}};
        Target target[3]{{&world[0]},{&world[1]},{&world[2]}};
        actor.m2C_models.push_back({target,3});actor.m1BC_isLead2=lead;
        actor.m158_formation=&formation;
        actor.m_lagiGroup.activeMask=&formation.mF0_flag;actor.m_lagiGroup.reactionMask=&formation.mF1_pad5[1];
        const unsigned idx=lead?2:0;const u32 quad=lead?0x060AFE44:0x060AFAFC;
        calls.clear();sounds.clear();randomCalls=0;animationSteps=0;positionUpdates=0;
        updateAttackStages(&actor);
        assert(actor.m28_state==0&&calls.empty()&&randomCalls==0);
        actor.m14_flags=0x04000000;updateAttackStages(&actor);
        assert(actor.m28_state==1&&calls.size()==1&&randomCalls==1&&formation.mF1_pad5[1]==2);
        const auto& charge=calls[0];
        assert(charge.charge&&charge.parent==&actor&&charge.sourcePtr==&world[idx]);
        assert(charge.state==&actor.m28_state&&charge.active==1&&charge.quad.address==quad&&charge.slot==6);
        updateAttackStages(&actor);assert(calls.size()==1&&actor.m28_state==1);
        engine.m1A0_battleAutoScrollDelta={7,-9,13};world[idx][0]+=111;
        actor.m14_flags=0x02000000;updateAttackStages(&actor);
        assert(actor.m28_state==2&&calls.size()==2&&randomCalls==1);
        const auto& shot=calls[1];
        assert(!shot.charge&&shot.parent==&actor&&shot.source==world[idx]);
        assert((shot.velocity==sVec3_FP{7,-9,0}));
        assert(shot.quad.address==quad&&shot.slot==6&&shot.damage==(lead?33:24)&&shot.flags==(lead?8:0));
        assert((sounds==std::vector<int>{0x69})&&animationSteps==4&&positionUpdates==4);
        updateAttackStages(&actor);assert(calls.size()==2&&sounds.size()==1);
    }
    std::puts("Single actor native stages: signal gates, world hotpoints, charge ownership, RNG and shot arguments PASS");
}

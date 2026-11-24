#include "lagi/native_battle_spacing.h"
static void lagiUpdateGarilDualSpacing(sBTL_A5_DualFormation* formation) {
    auto& group=formation->m1D0_entityGroup2;
    lagi::stepNativeBattleSpacing(formation->m_lagiSinglePositions,group.mE_numEntities,
        [&](int i) {
            s32 dead;std::memcpy(&dead,formation->m1E4_pad6+i*sizeof(dead),sizeof(dead));
            return !dead && lagiVisitFormationActor(group,i,[](p_workArea){});
        },[](){return randomNumber();},
        [](const sVec3_FP& a,const sVec3_FP& b){return vecDistance(a,b).asS32();});
}

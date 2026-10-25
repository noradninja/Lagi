#include "lagi/lagi_azel_upstream_prelude.h"
#include "lagi/battle_startup.h"
#include "lagi/battle_loading_cover.h"
#include "lagi/platform.h"
#include "lagi/disc_image.h"
#include "battle/battleManager.h"
#include "battle/battleGenericData.h"
#include "battle/BTL_A3/BTL_A3.h"
#include "battle/BTL_A3_2/BTL_A3_2.h"
#include "battle/BTL_T0/BTL_T0.h"
#include "commonOverlay.h"

namespace {
std::string startupError;
int genericDisc=-1;
bool reject(const std::string& error)
{
    if(startupError!=error){
        lagi::platform::logging::writef("[BattleStartup] waiting: %s\n",error.c_str());
        startupError=error;
    }
    return false;
}
}

bool lagiPrepareNativeA3Battle()
{
    const auto& fade=g_fadeControls.m0_fade0;
    const bool startingCoveredLoad=lagi::battleLoadingCover.active && !lagi::battleLoadingCover.loading;
    if(!lagi::battleLoadingCover.allowLoad(fade.m20_stopped,
        fade.m0_color[0].getInteger(),fade.m0_color[1].getInteger(),fade.m0_color[2].getInteger()))
        return false; // Keep publishing the outgoing scene while Azel fades it.
    if(startingCoveredLoad)
        lagi::platform::logging::writef("[BattleLoadingCover] loadUnderWhite=1\n");
    if(!gBattleManager || !gCommonFile)return reject("native manager/COMMON unavailable");
    const int overlay=gBattleManager->m2_currentBattleOverlayId;
    const int sub=gBattleManager->m6_subBattleId;
    if(overlay<0 || static_cast<std::size_t>(overlay)>=gCommonFile->battleOverlaySetup.size())
        return reject("overlay index outside COMMON table");
    const auto& entry=gCommonFile->battleOverlaySetup[overlay];
    if(entry.m4_prg!="BTL_A3.PRG" && entry.m4_prg!="BTL_A3_2.PRG" && entry.m4_prg!="BTL_T0.PRG")return reject("overlay not integrated: "+entry.m4_prg);
    if(sub<0 || static_cast<unsigned>(sub)>=entry.mC_numSubBattles)
        return reject("subbattle index outside authored overlay");
    if(entry.m4_prg=="BTL_T0.PRG" && getFileSize("BTL_T0.PRG")<392928u)
        return reject("BTL_T0.PRG missing/truncated");
    const int disc=lagi::disc::selected_disc();
    if(g_BTL_GenericData && genericDisc>=0 && disc!=genericDisc)
        return reject("disc changed; generic-data cache lifecycle needs restoration");
    if(!g_BTL_GenericData){
        if(getFileSize("BTL_A3.PRG")<lagi::battle_generic_minimum_bytes)
            return reject("BTL_A3.PRG missing/truncated");
        BTL_GenericData::staticInit();
        if(!g_BTL_GenericData || !g_BTL_GenericData->m_data ||
            g_BTL_GenericData->m_dataSize<lagi::battle_generic_minimum_bytes){
            if(g_BTL_GenericData){delete[] g_BTL_GenericData->m_data;delete g_BTL_GenericData;g_BTL_GenericData=nullptr;}
            return reject("BTL_A3.PRG read failed");
        }
        genericDisc=disc;
        lagi::platform::logging::writef("[BattleStartup] generic data ready disc=%d bytes=%u\n",disc,g_BTL_GenericData->m_dataSize);
    }
    return true;
}

bool lagiEnterNativeA3Battle(s_workArea* parent)
{
    // Native entry owns overlay data, current-overlay selection and creation
    // of the music/wait/engine task. No substitute scene or debug stats.
    const auto& name=gCommonFile->battleOverlaySetup[gBattleManager->m2_currentBattleOverlayId].m4_prg;
    s_workArea* task=nullptr;
    if(name=="BTL_A3.PRG") task=overlayStart_BTL_A3(parent);
    else if(name=="BTL_A3_2.PRG") task=overlayStart_BTL_A3_2(parent);
    else if(name=="BTL_T0.PRG") task=overlayStart_BTL_T0(parent);
    else return reject("overlay entry not integrated: "+name);
    if(!task)
        return reject("native battle main-task allocation failed: "+name);
    lagi::platform::logging::writef("[BattleStartup] enter %s overlay=%d sub=%d parent=%p\n",name.c_str(),
        int(gBattleManager->m2_currentBattleOverlayId),int(gBattleManager->m6_subBattleId),static_cast<void*>(parent));
    startupError.clear();
    return true;
}

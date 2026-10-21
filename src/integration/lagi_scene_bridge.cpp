#include "lagi/lagi_scene_bridge.h"

#include "lagi/lagi_azel_upstream_prelude.h"
#include "lagi/lagi_town_tasks.h"
#include "lagi/lagi_render_bridge.h"
#include "lagi/platform.h"
#include "lagi/native_scene_mode.h"
#if LAGI_FIELD_MOTION_TRACE
#include "lagi/motion_cadence.h"
#include <psp2/kernel/processmgr.h>
#endif

#include "kernel/moduleManager.h"
#include "field.h"
#include "field/fieldCamera.h"
#include "field/fieldDragon.h"
#include "field/fieldVisibilityGrid.h"

namespace lagi::scene_bridge {

void capture_local_coordinates(std::int16_t& x, std::int16_t& y)
{
    x = graphicEngineStatus.m405C.m44_localCoordinatesX;
    y = graphicEngineStatus.m405C.m46_localCoordinatesY;
}

namespace {
#if LAGI_FIELD_MOTION_TRACE
lagi::MotionCadence<12,3> g_fieldMotion;
#endif

void sync_battle_presentation_state()
{
    // No field-task/dragon pointer is required for module mode 4. Azel's grid
    // owns the view; the neutral fallback camera supplies no extra transform.
    static const float origin[3]={0.0f,0.0f,0.0f};
    static const float target[3]={0.0f,0.0f,1.0f};
    static const float up[3]={0.0f,1.0f,0.0f};
    lagi::platform::renderer::presentation_set_scene_mode(4u);
    const float nearPlane=static_cast<float>(static_cast<s32>(graphicEngineStatus.m405C.m10_nearClipDistance))/65536.0f;
    const float farPlane=static_cast<float>(static_cast<s32>(graphicEngineStatus.m405C.m14_farClipDistance))/65536.0f;
    lagi::platform::renderer::presentation_set_clip_planes(nearPlane,farPlane);
    std::int32_t nativeView[12]{};
    if(lagi::azel_bridge::native_scene_view_matrix(nativeView))
        lagi::platform::renderer::presentation_set_native_view_matrix(nativeView);
    lagi::platform::renderer::presentation_set_camera(origin,origin,target,up,0.0f,0.0f,0.0f);
}

void sync_field_presentation_state()
{
    if (!fieldTaskPtr || !fieldTaskPtr->m8_pSubFieldData)
        return;

    s_FieldSubTaskWorkArea* const field = fieldTaskPtr->m8_pSubFieldData;
    if (!field->m338_pDragonTask || !field->m334)
        return;

    // Phase 4 presentation boundary:
    // the render bridge removes Azel's native field view from mode-3 model
    // submissions, and Neptune reapplies that exact view matrix once. Static
    // environment-grid submissions can now remain in stable world space while
    // task-owned actors/effects continue to be classified dynamic.
    static const float cameraPosition[3] = {0.0f, 0.0f, 0.0f};
    static const float cameraTarget[3] = {0.0f, 0.0f, 1.0f};
    static const float cameraUp[3] = {0.0f, 1.0f, 0.0f};

    lagi::platform::renderer::presentation_set_scene_mode(3u);
    const float nearPlane =
        static_cast<float>(
            static_cast<s32>(graphicEngineStatus.m405C.m10_nearClipDistance)) /
        65536.0f;
    const float farPlane =
        static_cast<float>(
            static_cast<s32>(graphicEngineStatus.m405C.m14_farClipDistance)) /
        65536.0f;
    lagi::platform::renderer::presentation_set_clip_planes(
        nearPlane, farPlane);

    std::int32_t nativeView[12]{};
    if (lagi::azel_bridge::native_scene_view_matrix(nativeView))
        lagi::platform::renderer::presentation_set_native_view_matrix(
            nativeView);

    lagi::platform::renderer::presentation_set_camera(
        cameraPosition,
        cameraPosition,
        cameraTarget,
        cameraUp,
        0.0f,
        0.0f,
        0.0f);

    const s_dragonTaskWorkArea* const dragon = field->m338_pDragonTask;
#if LAGI_FIELD_MOTION_TRACE
    // Independent producer evidence: no live Azel pointers cross to renderer.
    auto& motion=g_fieldMotion;
    const auto serial=lagi::azel_bridge::published_frame_number()+1;
    static unsigned budget=24;
    std::array<std::int32_t,12> view{};
    const bool viewValid=lagi::azel_bridge::native_scene_view_matrix(view.data());
    std::array<std::int32_t,3> actor{};
    for(unsigned i=0;i<3;++i) {
        actor[i]=dragon->m8_pos[i].asS32();
    }
    const auto* regs=vdp2Controls.m4_pendingVdp2Regs;
    const unsigned rbg=regs && (regs->m20_BGON&0x10) && (regs->mFC_PRIR&7);
    const unsigned key=(unsigned(fieldTaskPtr->m2C_currentFieldIndex)&0xffu)<<16 |
        (unsigned(fieldTaskPtr->m2E_currentSubFieldIndex)&0xffu)<<8 | (rbg?1u:0u);
    if(budget && viewValid && motion.add(key,serial,sceKernelGetProcessTimeWide(),view,actor)) {
        const auto& s=motion.summary;
        lagi::platform::logging::writef(
            "[FieldMotion] diagnostic=1 key=%u serial=%llu intervals=%u minUs=%llu maxUs=%llu meanUs=%llu viewRepeated=%u positionRepeated=%u viewChangedPositionRepeated=%u serialGaps=%u minPositionStep=%llu maxPositionStep=%llu\n",
            key,(unsigned long long)serial,s.intervals,(unsigned long long)s.minUs,
            (unsigned long long)s.maxUs,(unsigned long long)(s.totalUs/s.intervals),
            s.aRepeated,s.bRepeated,s.aChangedBRepeated,s.serialGaps,
            (unsigned long long)s.minBStep,(unsigned long long)s.maxBStep);
        --budget;motion.nextWindow();
    }
    if(!viewValid)motion.reset();
#endif
    const s_visibilityGridWorkArea* const grid =
        field->m348_pFieldCameraTask1;

    unsigned int activeCells = 0u;
    int gridWidth = 0;
    int gridHeight = 0;
    if (grid && grid->m30) {
        gridWidth = grid->m30->m10_gridSize[0];
        gridHeight = grid->m30->m10_gridSize[1];
        const int cellCount = gridWidth * gridHeight;
        if (grid->m3C_cellRenderingTasks) {
            for (int i = 0; i < cellCount; ++i) {
                s_visdibilityCellTask* const cell =
                    grid->m3C_cellRenderingTasks[i];
                if (cell && cell->getTask() &&
                    !cell->getTask()->isPaused())
                    ++activeCells;
            }
        }
    }

    const int cameraCellX =
        grid ? grid->m18_cameraGridLocation[0] : -999;
    const int cameraCellY =
        grid ? grid->m18_cameraGridLocation[1] : -999;
    static int previousCameraCellX = -1000;
    static int previousCameraCellY = -1000;
    static unsigned int previousActiveCells = 0xFFFFFFFFu;
    if (cameraCellX != previousCameraCellX ||
        cameraCellY != previousCameraCellY ||
        activeCells != previousActiveCells) {
        lagi::platform::logging::writef(
            "[FieldCellTransition] cameraCell=(%d,%d) activeCells=%u "
            "gridSeen=%u gridVisible=%u submissions=%u\n",
            cameraCellX,
            cameraCellY,
            activeCells,
            grid ? static_cast<unsigned int>(grid->m12E0) : 0u,
            grid ? static_cast<unsigned int>(grid->m12E2) : 0u,
            static_cast<unsigned int>(
                lagi::azel_bridge::submission_count()));
        previousCameraCellX = cameraCellX;
        previousCameraCellY = cameraCellY;
        previousActiveCells = activeCells;
    }

    static unsigned int heartbeat = 0u;
    if ((heartbeat++ % 60u) == 0u) {
        lagi::platform::logging::writef(
            "[LagiFieldAdapter] field=%d sub=%d submissions=%u "
            "dragon=(%08X,%08X,%08X) angle=(%08X,%08X,%08X) "
            "grid=%dx%d cameraCell=(%d,%d) activeCells=%u "
            "gridSeen=%u gridVisible=%u renderMode=%u "
            "clip=(%.4f,%.4f)\n",
            static_cast<int>(fieldTaskPtr->m2C_currentFieldIndex),
            static_cast<int>(fieldTaskPtr->m2E_currentSubFieldIndex),
            static_cast<unsigned int>(
                lagi::azel_bridge::submission_count()),
            static_cast<unsigned int>(dragon->m8_pos[0].asS32()),
            static_cast<unsigned int>(dragon->m8_pos[1].asS32()),
            static_cast<unsigned int>(dragon->m8_pos[2].asS32()),
            static_cast<unsigned int>(dragon->m20_angle[0].asS32()),
            static_cast<unsigned int>(dragon->m20_angle[1].asS32()),
            static_cast<unsigned int>(dragon->m20_angle[2].asS32()),
            gridWidth,
            gridHeight,
            cameraCellX,
            cameraCellY,
            activeCells,
            grid ? static_cast<unsigned int>(grid->m12E0) : 0u,
            grid ? static_cast<unsigned int>(grid->m12E2) : 0u,
            grid ? static_cast<unsigned int>(grid->m12F2_renderMode) : 0u,
            nearPlane,
            farPlane);
    }
}

} // namespace

void sync_presentation_state()
{
#if LAGI_FIELD_MOTION_TRACE
    if(gGameStatus.m0_gameMode!=3)g_fieldMotion.reset();
#endif
    lagi::platform::renderer::presentation_set_native_frame_vblanks(
        vblankData.m14_numVsyncPerFrame);
    // Every native scene consumes the same Azel-authored UI planes. Do not
    // gate subtitles, badges, LCS backing or cinematic bars on actor type.
    if (lagi::native_scene_mode(gGameStatus.m0_gameMode)) {
        // The task sets menuId in entry state 0; setupVdp2ForMenu runs in
        // state 1 on a later frame. Never decode the previous scene's VRAM
        // with the specialized status-menu tile format during that gap.
        const auto* regs = vdp2Controls.m4_pendingVdp2Regs;
        const bool statusLayout = regs &&
            ((regs->m28_CHCTLA & 0x3171u) == 0x0101u) &&
            ((regs->m30_PNCN0 & 0xC000u) == 0x8000u) &&
            ((regs->m32_PNCN1 & 0xC000u) == 0x8000u) &&
            ((regs->mE4_CRAOFA & 0x77u) == 0x66u) &&
            ((((regs->m3C_MPOFN & 7u) * 64u +
                (regs->m40_MPABN0 & 63u)) * 0x800u) == 0x71800u) &&
            (((((regs->m3C_MPOFN >> 4) & 7u) * 64u +
                (regs->m44_MPABN1 & 63u)) * 0x800u) == 0x71000u);
        lagi::platform::renderer::presentation_set_vdp2_text(
            getVdp2Vram(0), getVdp2Cram(0), getVdp2Vram(0x3E000),
            statusLayout ? static_cast<unsigned int>(graphicEngineStatus.m40AC.m0_menuId) : 0u,
            false,
            graphicEngineStatus.m40BC_layersConfig[0].scrollX,
            graphicEngineStatus.m40BC_layersConfig[0].scrollY,
            graphicEngineStatus.m40BC_layersConfig[1].scrollX,
            graphicEngineStatus.m40BC_layersConfig[1].scrollY,
            graphicEngineStatus.m40BC_layersConfig[3].scrollX,
            graphicEngineStatus.m40BC_layersConfig[3].scrollY);
        if (regs) {
            lagi::SceneNbg0State nbg;
            nbg.enabled=(regs->m20_BGON&1) && !statusLayout &&
                lagi::sceneNbg0Supported(regs->m28_CHCTLA,regs->m30_PNCN0,regs->mEC_CCCTL,regs->m3A_PLSZ) &&
                !(regs->m9A_SCRCTL&1) && !(regs->mD0_WCTLA&0x20);
            nbg.additive=(regs->mEC_CCCTL&0x101)==0x101;
            auto& p=nbg.gpu;
            p[0][0]=regs->m30_PNCN0&255;p[0][1]=regs->mE4_CRAOFA&7;
            p[0][2]=(regs->m20_BGON>>8)&1;
            const unsigned maps[4]={unsigned(regs->m40_MPABN0)&63u,(unsigned(regs->m40_MPABN0)>>8)&63u,
                unsigned(regs->m42_MPCDN0)&63u,(unsigned(regs->m42_MPCDN0)>>8)&63u};
            for(unsigned i=0;i<4;++i)p[1][i]=lagi::sceneNbg0MapBase(regs->m3C_MPOFN,maps[i],regs->m3A_PLSZ);
            p[2][0]=float(regs->m70_SCXN0)/65536.0f;p[2][1]=float(regs->m74_SCYN0)/65536.0f;
            p[2][2]=float(regs->m78_ZMXN0)/65536.0f;p[2][3]=float(regs->m7C_ZMYN0)/65536.0f;
            const auto lineAddress=reinterpret_cast<std::uintptr_t>(regs->mA0_LSTA0);
            const auto vramBase=reinterpret_cast<std::uintptr_t>(getVdp2Vram(0));
            p[3][0]=(lineAddress>=vramBase && lineAddress<vramBase+0x80000)?float(lineAddress-vramBase):0.0f;
            p[3][1]=regs->m9A_SCRCTL&15;p[3][2]=1u<<((regs->m9A_SCRCTL>>4)&3);
            p[4][0]=(regs->mC0_WPSX0>>1)&511;p[4][1]=regs->mC2_WPSY0&511;
            p[4][2]=(regs->mC4_WPEX0>>1)&511;p[4][3]=regs->mC6_WPEY0&511;
            p[5][0]=(regs->mC8_WPSX1>>1)&511;p[5][1]=regs->mCA_WPSY1&511;
            p[5][2]=(regs->mCC_WPEX1>>1)&511;p[5][3]=regs->mCE_WPEY1&511;
            p[6][0]=regs->mD0_WCTLA&255;
            p[6][1]=((regs->mD8_LWTA0&0x7ffff)*2u)&0x7ffff;
            p[6][2]=((regs->mDC_LWTA1&0x7ffff)*2u)&0x7ffff;
            p[6][3]=((regs->mD8_LWTA0>>31)&1)|((regs->mDC_LWTA1>>30)&2);
            p[7][0]=(regs->m3A_PLSZ&1)?1024:512;
            p[7][1]=(regs->m3A_PLSZ&2)?1024:512;
            p[7][2]=regs->mF8_PRINA&7;
            // This overlay pass lies above primary world sprites and below
            // NBG1. Other priority bands need their own compositor stage.
            nbg.enabled=nbg.enabled && lagi::sceneNbg0OverlayBand(regs->m20_BGON,regs->mF8_PRINA,regs->mF0_PRISA);
            lagi::platform::renderer::presentation_set_scene_nbg0(nbg);
            static unsigned nbgReports=0;
            static unsigned nbgPrevious=~0u;
            const unsigned nbgKey=(unsigned(regs->m28_CHCTLA)<<16)|regs->m20_BGON;
            if((regs->m20_BGON&1) && nbgKey!=nbgPrevious && nbgReports<12) {
                ++nbgReports;nbgPrevious=nbgKey;
                lagi::platform::logging::writef("[SceneNBG0State] enabled=%u BGON=%04X CHCTLA=%04X PNCN0=%04X SCRCTL=%04X WCTLA=%04X CCCTL=%04X PRINA=%04X line=%u\n",
                    nbg.enabled?1u:0u,regs->m20_BGON,regs->m28_CHCTLA,regs->m30_PNCN0,
                    regs->m9A_SCRCTL,regs->mD0_WCTLA,regs->mEC_CCCTL,regs->mF8_PRINA,unsigned(p[3][0]));
            }
            lagi::Vdp2SpriteColorState sprite;
            sprite.ccctl=regs->mEC_CCCTL;
            sprite.bgon=regs->m20_BGON; sprite.prir=regs->mFC_PRIR;
            sprite.spctl=regs->mE0_SPCTL;
            sprite.craofb=regs->mE6_CRAOFB;
            sprite.priority[0]=regs->mF0_PRISA; sprite.priority[1]=regs->mF2_PRISB;
            sprite.priority[2]=regs->mF4_PRISC; sprite.priority[3]=regs->mF6_PRISD;
            sprite.ratio[0]=regs->m100_CCRSA; sprite.ratio[1]=regs->m102_CCRSB;
            sprite.ratio[2]=regs->m104_CCRSC; sprite.ratio[3]=regs->m106_CCRSD;
            sprite.colorWindow=regs->mD6_WCTLD>>8;
            sprite.window[0]=regs->mC0_WPSX0; sprite.window[1]=regs->mC2_WPSY0;
            sprite.window[2]=regs->mC4_WPEX0; sprite.window[3]=regs->mC6_WPEY0;
            sprite.window[4]=regs->mC8_WPSX1; sprite.window[5]=regs->mCA_WPSY1;
            sprite.window[6]=regs->mCC_WPEX1; sprite.window[7]=regs->mCE_WPEY1;
            lagi::platform::renderer::presentation_set_sprite_color_state(sprite);
        }
        if (statusLayout) {
            lagi::MenuNbg2State map;
            map.bgon=regs->m20_BGON;
            map.chctlb=regs->m2A_CHCTLB;
            map.pncn=regs->m34_PNCN2;
            map.plsz=regs->m3A_PLSZ;
            map.mpofn=regs->m3C_MPOFN;
            map.mpab=regs->m48_MPABN2;
            map.mpcd=regs->m4A_MPCDN2;
            map.craofa=regs->mE4_CRAOFA;
            map.prina=regs->mF8_PRINA;
            map.prinb=regs->mFA_PRINB;
            map.wctlb=regs->mD2_WCTLB;
            map.ccctl=regs->mEC_CCCTL;
            map.ccrnb=regs->m10A_CCRNB;
            map.window[0]=regs->mC0_WPSX0;
            map.window[1]=regs->mC2_WPSY0;
            map.window[2]=regs->mC4_WPEX0;
            map.window[3]=regs->mC6_WPEY0;
            map.scrollX=static_cast<std::int16_t>(graphicEngineStatus.m40BC_layersConfig[2].scrollX);
            map.scrollY=static_cast<std::int16_t>(graphicEngineStatus.m40BC_layersConfig[2].scrollY);
            lagi::platform::renderer::presentation_set_menu_nbg2(map);
        }
    }
    switch (gGameStatus.m0_gameMode) {
    case 1:
    case 2:
        // Town remains Azel-owned. The adapter exposes only renderer-facing
        // state produced by Azel's native town task graph.
        lagi::azel::twn_ruin_sync_platform_state();
        {
            std::int32_t nativeView[12]{};
            if (lagi::azel_bridge::native_scene_view_matrix(nativeView))
                lagi::platform::renderer::presentation_set_native_view_matrix(nativeView);
        }
        break;
    case 3:
        // Field remains Azel-owned. Phase 2 only opens the generic
        // presentation boundary for its existing model submissions.
        sync_field_presentation_state();
        break;
    case 4:
        sync_battle_presentation_state();
        break;
    default:
        break;
    }
}

} // namespace lagi::scene_bridge

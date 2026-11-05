// CAMP060555E2/06055634 and060557BC; included before the translated stub.
// Azel retains animation dispatch, script polling and player-control release.
#include "lagi_camp_recovery_effect.inc"

void sCampDragon_Draw(sTownDragon*);
int scriptFunction_0606c952(); // Native no-op camera update0606B396.

static s32 lagiCampRecoveryDivide(s32 n,s32 d){return FP_Div(n,d).asS32();}
static s32 lagiCampRecoveryMultiply(s32 a,s32 b){return MTH_Mul(a,b).asS32();}

static void lagiCampRecoveryDraw(sTownDragon* base)
{
    auto* dragon=static_cast<sCampDragon*>(base);
    const u32 color=lagi::camp::recovery::step_recovery_light(dragon->lagiRecoveryLight,
        dragon->m608_savedLightColor[0].asS32(),lagiCampRecoveryDivide,lagiCampRecoveryMultiply);
    setupLightColor(color);
    sCampDragon_Draw(base);
    generateLightFalloffMap(dragon->m608_savedLightColor[0].asS32(),
        dragon->m608_savedLightColor[1].asS32(),dragon->m608_savedLightColor[2].asS32());
}

static s32 lagiCampRecoveryUpdate(sCampDragon* dragon)
{
    lagi::camp::recovery::step_sequence(dragon->lagiRecoverySequence,
        lagiCampRecoveryDivide,
        [&](s32 factor){
            const auto& saved=dragon->m5E4_savedCameraPosition;
            const auto& camera=dragon->m5F0_savedCameraTarget;
            const auto target=lagi::camp::recovery::interpolate_target(
                {saved[0].asS32(),saved[1].asS32(),saved[2].asS32()},
                {camera[0].asS32(),camera[1].asS32(),camera[2].asS32()},
                factor,lagiCampRecoveryMultiply);
            // Native0606CA50 stores the target and disables camera updates.
            twnMainLogicTask->m44_cameraTarget={target[0],target[1],target[2]};
            scriptFunction_0606c952();
        },
        [&](){sCampDragon_UpdateMode4Mode9(dragon,10);},
        [&](){return dragon->m14_readyState==10;},
        [&](){
            lagi::camp::recovery::begin_recovery_light(dragon->lagiRecoveryLight,
                dragon->m608_savedLightColor[0].asS32());
            dragon->m_DrawMethod=&lagiCampRecoveryDraw;
        },
        [&](){
            playSystemSoundEffect(0x6F);
            lagiCreateCampRecoveryEffect(dragon,gCurrentTownOverlay->getSaturnPtr(0x0607B052),
                &dragon->m58_position,0x1800,4,0x23,0x8000,1);
            sCampDragon_startAnimSequence(dragon,gCurrentTownOverlay->getSaturnPtr(0x0607AF5E));
        },
        [&](){dragon->m_DrawMethod=&sCampDragon_Draw;dragon->m11_subState=0;});
    return 0;
}

// CAMP060559A4. Native mE dispatch prevents phase2 until the animation command
// stream has completed; do not replace that wait with an independent timer.
static s32 lagiCampFeedingUpdate(sCampDragon* dragon)
{
    lagi::camp::recovery::step_feeding(dragon->lagiRecoverySequence,
        [&](){sCampDragon_UpdateMode4Mode9(dragon,10);},
        [&](){return dragon->m14_readyState==10;},
        [&](){
            lagi::camp::recovery::begin_recovery_light(dragon->lagiRecoveryLight,
                dragon->m608_savedLightColor[0].asS32());
            dragon->lagiRecoveryLight.highlight=0x00001F00;
            dragon->m_DrawMethod=&lagiCampRecoveryDraw;
            playSystemSoundEffect(0x6F);
        },
        [&](){sCampDragon_startAnimSequence(dragon,gCurrentTownOverlay->getSaturnPtr(0x0607AF5E));},
        [&](){dragon->m_DrawMethod=&sCampDragon_Draw;dragon->m11_subState=0;});
    return 0;
}

// Native FLD_A5 0605AFAC and 0605B694. Named host fields only.
static void lagiOasisMonolithActivationUpdate(sA5SkyEntity* actor) {
    actor->m11C_flag=!checkPositionVisibilityAgainstFarPlane(&actor->mE8_position);
    actor->m108+=actor->m10C;actor->m104+=actor->m108;
    ++actor->m114;actor->mF4-=actor->mF4>>2;
    if(actor->m114>9) {
        if(actor->m114<60) {if(!(actor->m114&7))lagiOasisMonolithRing(actor,true);}
        else if(actor->m114<84) {
            const auto chance=randomNumber();
            if(!(chance&15)||!(actor->m114&15)) {actor->mF4=s32(randomNumber()&0x1FFF)-0x100;lagiOasisMonolithBurst(actor);}
            if(!(actor->m114&7))lagiOasisMonolithRing(actor,false);
            lagiOasisMonolithRing(actor,true);
        }else {
            const auto chance=randomNumber();
            if(!(chance&7)||!(actor->m114&7)) {actor->mF4=s32(randomNumber()&0x3FFF)-0x200;lagiOasisMonolithBurst(actor);}
            lagiOasisMonolithRing(actor,false);
        }
    }
    if(actor->m114==24)lagiOasisMonolithBurst(actor);
    if(actor->m114<30)actor->m10C+=0x7482;else actor->m10C=0;
    if(actor->m114>=120) {
        actor->m98_model2=s_3dModel{};
        lagiOasisMonolithSetupDebris(actor,false);
        actor->mF8_rotation[1]-=actor->m104;
        lagiOasisMonolithBurst(actor);lagiOasisMonolithFinalRings(actor);
        mainGameState.bitField[0x95]|=2;
        playPCM(actor,0x70);playSystemSoundEffect(0x73);
        actor->m114=240;actor->m_UpdateMethod=&lagiOasisMonolithSettledUpdate;
    }
}
static void lagiOasisMonolithActivate(sA5SkyEntity* actor,sFieldModelRenderContext*) {
    startFieldScript(5,0x5BA);
    mainGameState.bitField[0x96]|=0x20;
    actor->mF4=0x333;actor->m114=0;actor->m104+=0x16C16C;
    actor->m108+=actor->m10C;actor->m104+=actor->m108;
    actor->m_UpdateMethod=&lagiOasisMonolithActivationUpdate;
    lagiOasisMonolithBurst(actor);
    a5CutsceneCameraInit(actor,gFLD_A5->getSaturnPtr(0x0608C8AC),0x0605B58A,&actor->mE8_position,0);
}

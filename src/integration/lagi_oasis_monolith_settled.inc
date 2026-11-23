// FLD_A5 0605AD7A, 0605AEF4 and normal (non-debug) 0605B1FC.
static void lagiOasisMonolithSettledDraw(sA5SkyEntity* actor) {
    if(!actor->m11C_flag)return;
    pushCurrentMatrix();translateCurrentMatrix(&actor->mE8_position);
    rotateCurrentMatrixZYX(&actor->mF8_rotation);
    actor->m48_model1.m18_drawFunction(&actor->m48_model1);
    popMatrix();
}
static void lagiOasisMonolithSettledUpdate(sA5SkyEntity* actor) {
    actor->m11C_flag=!checkPositionVisibilityAgainstFarPlane(&actor->mE8_position);
    if(actor->m114 && --actor->m114<=0) {
        actor->m114=0;
        if(findSound(0x70)>=0)enqueuePlaySoundEffect(0x70,5,0,2);
    }
}
static void lagiOasisMonolithSetupDebris(sA5SkyEntity* actor,bool reverse) {
    auto* bundle=actor->m0_memoryArea.m0_mainMemoryBundle;
    sDebrisScatterParams config;
    initDebrisScatterConfig(&config,0x18,0xB50);
    auto* raw=bundle->getRawBuffer();
    config.m8_spread=readMaxScalarFromBundleTree(raw,READ_BE_U32(raw+0x18));
    actor->mF8_rotation[1]+=actor->m104;
    config.m10_pPosition=&actor->mE8_position;
    config.m14_pRotation=&actor->mF8_rotation;
    config.m28_rotVelY=actor->m108>>1;
    config.m30_angleOffset=s32(u32(actor->m108)<<4);
    config.m38=0xA000;config.m44_soundEffect=0x6F;
    config.m_pBundle=bundle;
    actor->lagiDebris=createDebrisScatterTask(actor,&config,reverse);
    actor->m_DrawMethod=&lagiOasisMonolithSettledDraw;
}

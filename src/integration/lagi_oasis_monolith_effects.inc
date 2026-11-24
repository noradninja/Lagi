// Native FLD_A5 0605B900/B9C8/BAE0/BB9C: original shared effect producers.
static void lagiOasisMonolithBurst(sA5SkyEntity* actor) {
    static const auto quads=initVdp1Quad(gFLD_A5->getSaturnPtr(0x0609A008));
    auto position=actor->mE8_position;position[1]+=0x26000;
    for(unsigned axis=0;axis<3;++axis)position[axis]+=s32(randomNumber()&0x1FFFF)-0x10000;
    sVec3_FP velocity{0,0xCC,0};sParticleSpawnConfig config{};config.m8_pQuadData=&quads;
    auto* field=static_cast<s_fieldSpecificData_A5*>(getFieldTaskPtr()->mC);
    spawnParticleProjected(field->m54_particlePool,&config,&position,&velocity);
    enqueuePlaySoundEffect(0x72,1,0x73,0); // native 0602C2D4
}
static void lagiOasisMonolithRing(sA5SkyEntity* actor,bool small) {
    const u32 angle=randomNumber()&0x0FFFFFFF;
    sAnimRingArg arg{};arg.m00_center=actor->mE8_position;
    if(small) {
        arg.m00_center[1]+=0x3E000;
        arg.m18_ring1=actor->mE8_position;arg.m18_ring1[1]=0xC000;
        arg.m24_direction[0]=MTH_Mul(0x17000,getCos(angle>>16));
        arg.m24_direction[1]=0xA000;
        arg.m24_direction[2]=MTH_Mul(0x17000,getSin(angle>>16));
        arg.m30_lifetime=12;arg.m34_mode=1;
    }else {
        arg.m00_center[1]+=0x3C000-s32(randomNumber()&0x1FFFF);
        arg.m0C_ring0=arg.m18_ring1=actor->mE8_position;
        arg.m0C_ring0[1]=arg.m18_ring1[1]=0;
        const auto cosine=getCos(angle>>16),sine=getSin(angle>>16);
        arg.m0C_ring0[0]+=MTH_Mul(0x5000,cosine);arg.m0C_ring0[2]+=MTH_Mul(0x5000,sine);
        arg.m18_ring1[0]+=MTH_Mul(0xF000,cosine);arg.m18_ring1[2]+=MTH_Mul(0xF000,sine);
        arg.m24_direction={MTH_Mul(0xF000,cosine),0x1E000,MTH_Mul(0xF000,sine)};
        arg.m30_lifetime=3;arg.m34_mode=0;
    }
    spawnAnimRingSubTask(actor,&arg);
}
static void lagiOasisMonolithFinalRings(sA5SkyEntity* actor) {
    for(u32 baseAngle=0;baseAngle<0x10000000;baseAngle+=0x2000000) {
        const u32 angle=(randomNumber()&0x1FFFFFF)+baseAngle;
        const s32 radius=s32((randomNumber()+0x32000u)&0x3FFFFu);
        sAnimRingArg arg{};arg.m00_center=actor->mE8_position;arg.m00_center[1]=0;
        arg.m18_ring1=actor->mE8_position;arg.m18_ring1[1]=0;
        arg.m18_ring1[0]+=MTH_Mul(radius,getCos(angle>>16));
        arg.m18_ring1[2]+=MTH_Mul(radius,getSin(angle>>16));
        arg.m30_lifetime=(randomNumber()&0x3F)+30;arg.m34_mode=1;
        spawnAnimRingSubTask(actor,&arg);
    }
}

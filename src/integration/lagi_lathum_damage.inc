// BTL_A5_3 06055C3C..06055EDE. Called after the mode controller in phase 1.
void CreateDamageSpriteForCurrentBattleOverlay(sVec3_FP*,sVec3_FP*,s32,s8);

static s16 lagiLathumDamageWord(s32 value) {
    const u32 bits=u32(value)&0xffffu;
    return s16(bits<0x8000u?s32(bits):s32(bits)-0x10000);
}

static void lagiUpdateLathumDamage(s_workAreaCopy* owner,LagiLathumRig& rig,
                                  s_battleEngine& engine) {
    if (rig.health<=0) return;
    for (unsigned index=0;index<rig.groups.size();++index) {
        auto& group=rig.groups[index];
        const auto anchor=readSaturnU8(g_BTL_A5_3->getSaturnPtr(0x060A3ED4+index));
        sVec3_FP world;
        // Target refresh uses last frame's hotpoints and PREVIOUS inverse
        // camera, whereas Draw uses the current inverse camera.
        transformAndAddVec(rig.corePositions[anchor],world,cameraProperties2.m28[1]);
        if (group.state) world[1]-=0xA000;
        transformAndAddVecByCurrentMatrix(&world,&group.position);
        for (auto& target:group.targets) {
            if (target.m50_flags&0x80000u) {
                if (rig.groupCounter<0) rig.groupCounter=4;
                if (index==rig.selectedGroup && rig.groupCounter>2) rig.groupCounter=2;
                rig.damagePending=1;
                createHitSparkEffect(owner,target,0);
                rig.pendingDamage=lagiLathumDamageWord(s32(rig.pendingDamage)+target.m58);
                rig.health=lagiLathumDamageWord(s32(rig.health)-target.m58);
                rig.lifeMeter->m31|=2;
                CreateDamageSpriteForCurrentBattleOverlay(getBattleTargetablePosition(target),nullptr,0x40000,1);
                playSystemSoundEffect(0x6C);
                target.m50_flags&=~0x80000u;
                target.m50_flags&=~0x20000u;
                if (index==rig.selectedGroup) {
                    const auto old=rig.selectedHitCount;
                    rig.selectedHitCount=u8(old+1);
                    if (!old) rig.mode=5;
                    group.state=2;
                } else group.state=1;
                battleEngine_FlagQuadrantForSafety(readSaturnU8(g_BTL_A5_3->getSaturnPtr(0x060A3EDD+index)));
            }
            switch (group.state) {
                case 0: target.m60=1;break;
                case 1: target.m60=3;break;
                case 2: target.m60=20;break;
            }
        }
    }
    if (rig.health<=0 || (readKeyboardToggle(0x81) && rig.mode!=4)) {
        rig.health=0;
        createDamageDisplayTask(owner,rig.pendingDamage,&engine.mC_battleCenter,1);
        rig.mode=0xB;rig.substate=0;
        // Native death leaves the accumulator and pending byte intact.
    } else if (rig.damagePending && engine.m188_flags.m1000) {
        createDamageDisplayTask(owner,rig.pendingDamage,&engine.mC_battleCenter,1);
        rig.pendingDamage=0;rig.damagePending=0;
    }
}

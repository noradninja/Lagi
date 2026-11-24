// Native shared model damage 060A36D6..060A3A4A.
void createExplosionAtPosition(sVec3_FP*,sVec3_FP*,int,u8);
static bool BTL_A5_enemy_checkDamage(sBTL_A5_Enemy* actor)
{
    // Native constructor initializes auxiliary count/pointer to zero.
    LagiBattleModelTargetState targets;
    targets.healthFloor=actor->mB4;
    targets.damageCallbackEA={actor->mB8_damageCallback0,actor->mBC_damageCallback1,
        actor->mC0_damageCallback2,actor->mC4_damageCallback3};
    targets.hitboxValues={actor->m144_hitbox0,actor->m146_hitbox1,
        actor->m148_hitbox2,actor->m14A_hitbox3};
    const LagiBattleModelDamageHooks hooks{nullptr,
        [](void*)->std::uint8_t {return gBattleManager->m10_battleOverlay->m4_battleEngine->m22C_dragonCurrentQuadrant;},
        [](void*) {return bool(gBattleManager->m10_battleOverlay->m4_battleEngine->m188_flags.m1000);},
        [](void*,std::uint32_t ea,s_workAreaCopy*,sBattleTargetable& target) {
            if (ea==0x060A3A4C) return;
            assert(ea==0x060A3A50 || ea==0x060A3A74 || ea==0x060A3A98);
            createExplosionAtPosition(target.mC,
                &gBattleManager->m10_battleOverlay->m4_battleEngine->m1A0_battleAutoScrollDelta,
                0x20000,ea==0x060A3A50 ? 1 : 0);
        },
        [](void*,s_workAreaCopy* owner,sBattleTargetable& t,int mode) {createHitSparkEffect(owner,t,mode);},
        [](void*,s_workAreaCopy* owner,std::int16_t n,sVec3_FP* p,int mode) {createDamageDisplayTask(owner,n,p,mode);}
    };
    return lagiUpdateBattleEnemyModelDamage({actor,actor->m14_flags,actor->mB2_health,
        actor->mB4,actor->mB6,actor->m14C_lightColor,
        actor->mAC_lifeMeter->m31,actor->m30_position,actor->m2C_models,targets},hooks);
}

// Native 060555D0..06055632. Four live damage values, not UI priority.
static void lagiUpdateGarilVariantDamageValues(sBTL_A5_VariantEnemy* actor) {
    const auto quadrant=actor->m158_formation->mF4_entityPositions[actor->m1C_entityIndex];
    if(quadrant>=4)return;
    actor->m_lagiTargets.hitboxValues.fill(5);
    actor->m_lagiTargets.hitboxValues[(quadrant+2)&3]=45;
}
void createExplosionAtPosition(sVec3_FP*,sVec3_FP*,int,u8);
static bool lagiUpdateGarilVariantDamage(sBTL_A5_VariantEnemy* actor)
{
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
        actor->m_lagiTargets.healthFloor,actor->m_lagiPendingDamage,actor->m14C_lightColor,
        actor->mAC_lifeMeter->m31,actor->m30_position,actor->m2C_models,actor->m_lagiTargets},hooks);
}

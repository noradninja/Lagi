// BTL_T0 0609F440..0609F4AE. The shared factory at 06074328 is
// createExplosionAtPosition, not createGunAndLaserDamage3dModel. Its callee
// 0607386C is the 0x44-byte animated-quad particle task (position, velocity,
// acceleration, scale, colors, lifetime), matching createParticleEffect.
bool lagiDispatchTutorialModelHit(std::uint32_t callbackEA, sBattleTargetable& target)
{
    unsigned mode;
    switch (callbackEA) {
    case 0x0609f440u: return true;
    case 0x0609f444u: mode=1; break;
    case 0x0609f468u:
    case 0x0609f48cu: mode=0; break;
    default: return false;
    }
    // Original callback loads target+C, not target's current hotpoint (+4).
    // Engine+1A0 is the existing typed battle auto-scroll velocity field.
    auto* engine=gBattleManager->m10_battleOverlay->m4_battleEngine;
    createExplosionAtPosition(target.mC,&engine->m1A0_battleAutoScrollDelta,
        0x20000,static_cast<u8>(mode));
    return true;
}

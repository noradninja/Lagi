// FLD_A3 0605E4E6..0605E5EA. The shootable cart has a separate state
// machine from the looping waypoint carts. Keep the native state-2 X read
// in its Z endpoint test (0605E594); do not silently "correct" authored logic.
static void Update2(s_A3_0_Obj3* pThis)
{
    updateLCSTarget(&pThis->m38_LCSTarget);
    switch (pThis->m6C) {
    case 0:
        break;
    case 1:
        pThis->m10_position[0] += pThis->m1C_velocity[0];
        pThis->m10_position[2] += pThis->m1C_velocity[2];
        A3_0_Obj3Update1Sub2(pThis->m10_position);
        if (std::abs(pThis->m10_position[0] - readSaturnS32(pThis->mC)) < 0x10000 &&
            std::abs(pThis->m10_position[2] - readSaturnS32(pThis->mC + 4)) < 0x10000) {
            createSmokePufTask(pThis, &pThis->m10_position, &pThis->m1C_velocity);
            ++pThis->m6C;
        }
        break;
    case 2:
        if (((pThis->m1C_velocity[0] < 0) !=
                (pThis->m10_position[0] >= readSaturnS32(pThis->mC))) &&
            ((pThis->m1C_velocity[2] < 0) !=
                (pThis->m10_position[0] >= readSaturnS32(pThis->mC + 4)))) {
            pThis->getTask()->markFinished();
        } else {
            pThis->m10_position[0] += pThis->m1C_velocity[0];
            pThis->m10_position[2] += pThis->m1C_velocity[2];
            A3_0_Obj3Update1Sub2(pThis->m10_position);
        }
        break;
    default:
        break; // Native dispatch falls through to proximity/visibility update.
    }
    A3_0_Obj3Update2Sub0(pThis->m10_position, pThis->m8->m4_position[1]);
    pThis->m6E_visible = !checkPositionVisibilityAgainstFarPlane(&pThis->m10_position);
}

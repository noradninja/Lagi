// BTL_A3_2 060A0EAA and 060A0DE4. Each segment has an authored
// width and (for the second entry point) its own four Gouraud offsets.
static void lagiDrawBattleTrail(sTrailRenderer* trail, bool gouraud)
{
    if (!trail->m0_positions || trail->m4_numPositions < 2) return;
    const auto data = trail->m10_renderData;
    const auto widths = readSaturnEA(data + 0xC);
    const auto colors = gouraud ? readSaturnEA(data + 0x10) : sSaturnPtr();
    for (s32 i = trail->m4_numPositions - 1; i > trail->m8_visibleCount; --i) {
        std::array<sVec3_FP, 2> points = {trail->m0_positions[i], trail->m0_positions[i - 1]};
        quadColor color = {0x4210, 0x4210, 0x4210, 0x4210};
        if (gouraud) for (unsigned corner = 0; corner < 4; ++corner)
            color[corner] = readSaturnU16(colors + (i - 1) * 8 + corner * 2);
        displayRaySegment(points, readSaturnS32(widths + (i - 1) * 4),
            trail->m14_vdp1Base + readSaturnU16(data + 4), readSaturnS16(data + 6),
            trail->m14_vdp1Base + readSaturnU16(data + 8), &color, 8);
    }
}
void trailRenderer_draw(sTrailRenderer* trail) { lagiDrawBattleTrail(trail, false); }
void lagiDrawBattleTrailGouraud(sTrailRenderer* trail) { lagiDrawBattleTrail(trail, true); }

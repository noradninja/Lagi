// Native 06054872 counts exactly the three authored quadrant bytes.
// Retired entries contain 4 and are deliberately absent from all quadrants.
static void lagiUpdateGarilVariantQuadrants(sBTL_A5_FormationTask* formation) {
    std::array<unsigned,4> counts{};
    for(unsigned actor=0;actor<3;++actor) {
        const auto quadrant=formation->mF4_entityPositions[actor];
        if(quadrant<4)++counts[quadrant];
    }
    battleEngine_FlagQuadrantBitForSafety(0);
    battleEngine_FlagQuadrantBitForDanger(0);
    u16 safety=0,danger=0;
    for(unsigned quadrant=0;quadrant<4;++quadrant) {
        const auto shift=readSaturnU32(g_BTL_A5->getSaturnPtr(0x060AD194+quadrant*4));
        const u16 mask=u16(1u<<shift);
        if(counts[quadrant]==0)danger|=mask;
        else if(counts[quadrant]!=1)safety|=mask;
    }
    battleEngine_FlagQuadrantBitForSafety(safety);
    battleEngine_FlagQuadrantBitForDanger(danger);
}

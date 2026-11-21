static void testNativeBattleChargeQuads()
{
    sVec3_FP position{};position[0]=13;position[1]=-7;position[2]=4;
    u8 state=1;
    const auto* cached=lagiProjectileQuad({0x600});
    const auto originalWidth=(*cached)[0].mC_width.m_value;
    const unsigned beforeRandom=draws;
    auto* a=static_cast<sLagiBattleChargeQuad*>(
        lagiCreateNativeBattleChargeQuad(&dragon,&position,&state,1,{0x600},6));
    auto* b=static_cast<sLagiBattleChargeQuad*>(
        lagiCreateNativeBattleChargeQuad(&dragon,&position,&state,1,{0x600},6));
    assert(a->frames!=b->frames&&a->frames!=cached&&b->frames!=cached);
    assert(draws==beforeRandom&&a->width.m_value==0&&a->height.m_value==0);
    for(int frame=1;frame<=25;++frame){
        ++position[0].m_value;
        lagiBattleChargeQuadUpdate(a);lagiBattleChargeQuadDraw(a);
        const s32 size=frame<=20 ? frame*0x333 : 0x4000;
        assert(a->width.m_value==size&&a->height.m_value==size);
        assert(a->position[0].m_value==position[0].m_value);
        assert(a->position[1].m_value==position[1].m_value+0x2000);
        assert(a->position[2].m_value==position[2].m_value+0x2000);
        const unsigned current=a->quad.m7_currentFrame;
        assert(current==unsigned(frame%2));
        assert((*a->frames)[current].mC_width.m_value==size);
        assert((*a->frames)[current].m10_height.m_value==size);
        const s32 previous=frame<=21 ? (frame-1)*0x333 : 0x4000;
        assert((*a->frames)[1-current].mC_width.m_value==previous);
        assert((*b->frames)[0].mC_width.m_value==originalWidth);
        assert((*cached)[0].mC_width.m_value==originalWidth);
    }
    state=2;a->source=nullptr; // state gate must precede retired hotpoint access
    const unsigned beforeQuad=quadSteps;
    lagiBattleChargeQuadUpdate(a);
    assert(a->control.finished&&quadSteps==beforeQuad);
    lagiBattleChargeQuadDelete(a);assert(!a->frames);delete a;
    lagiBattleChargeQuadDelete(b);delete b;
    assert(cached&&!cached->empty()&&(*cached)[0].mC_width.m_value==originalWidth);
    std::puts("Native charge quad: live hotpoint, scale clamp, private frames and retirement PASS");
}

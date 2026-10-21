// BTL_A3_2 0606D7A4 / 0606D9D0: speech writes the bottom text plane,
// not the boxed battle announcement task. Keep native text and duration.
struct LagiBattleSubtitle : s_workAreaTemplate<LagiBattleSubtitle> {
    s16 remaining=0;
    static void region() { setupVDP2StringRendering(3,25,38,4); }
    static void Update(LagiBattleSubtitle* self) {
        if (self->remaining-- < 0) self->getTask()->markFinished();
    }
    static void Delete(LagiBattleSubtitle*) { region(); clearVdp2TextArea(); }
};
static void lagiCreateBattleSubtitle(p_workArea parent,s16 index,s16 duration) {
    // 0606D7CA..0606D7EE resolves the current sub-battle entry each time.
    // The announcement task's cached text table may belong to an earlier
    // sub-battle; it is not the native speech lookup boundary.
    auto* engine=gBattleManager->m10_battleOverlay->m4_battleEngine;
    if (!engine || engine->m3A8_overlayBattledata.isNull()) return;
    const auto texts=readSaturnEA(engine->m3A8_overlayBattledata+
        static_cast<s8>(engine->m3B0_subBattleId)*0x20+0xC);
    if(texts.isNull()) return;
    static const LagiBattleSubtitle::TypedTaskDefinition definition={
        nullptr,&LagiBattleSubtitle::Update,nullptr,&LagiBattleSubtitle::Delete};
    auto* task=createSubTask<LagiBattleSubtitle>(parent,&definition);
    task->remaining=duration;
    vdp2StringContext.m0=0;
    LagiBattleSubtitle::region();
    clearVdp2TextArea();
    VDP2DrawString(readSaturnString(readSaturnEA(texts+index*4)).c_str());
}

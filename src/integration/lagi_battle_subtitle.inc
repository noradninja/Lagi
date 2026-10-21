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
    auto* display=gBattleManager->m10_battleOverlay->m14_textDisplay;
    if (!display || display->m0_texts.isNull()) return;
    static const LagiBattleSubtitle::TypedTaskDefinition definition={
        nullptr,&LagiBattleSubtitle::Update,nullptr,&LagiBattleSubtitle::Delete};
    auto* task=createSubTask<LagiBattleSubtitle>(parent,&definition);
    task->remaining=duration;
    vdp2StringContext.m0=0;
    LagiBattleSubtitle::region();
    VDP2DrawString(readSaturnString(readSaturnEA(display->m0_texts+index*4)).c_str());
}

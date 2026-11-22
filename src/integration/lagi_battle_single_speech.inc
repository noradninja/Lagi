// Shared chain behavior is unchanged; negative-flag native tasks use the
// same voice/text states but terminate at state2 instead of reading a table.
void sFormationIntroEffectTask::Update(sFormationIntroEffectTask* task) {
    formationIntroEffect_HandleSkipKey(task);
    lagiFormationIntroEffectUpdateBody(task);
}
static void lagiSingleBattleSpeechUpdate(sFormationIntroEffectTask* task) {
    formationIntroEffect_HandleSkipKey(task);
    if(task->m0_state==2) {task->getTask()->markFinished();return;}
    lagiFormationIntroEffectUpdateBody(task);
}
s_workArea* lagiCreateNativeBattleSpeech(s_workArea* parent,std::int16_t text,
    std::int16_t duration,sSaturnPtr filename) {
    static const sFormationIntroEffectTask::TypedTaskDefinition definition={
        nullptr,&lagiSingleBattleSpeechUpdate,nullptr,&sFormationIntroEffectTask::Delete};
    auto* task=createSubTaskWithArg<sFormationIntroEffectTask>(parent,sSaturnPtr{},&definition);
    if(!task)return nullptr;
    task->m0_state=0;task->m1_subState=0;task->m2_flag=-1;task->m3_entryIndex=0;
    task->m4_totalFrames=0x7FFFFFFF;task->m8_currentFrame=0;task->mC_remainingFrames=0;
    task->m10_battleStringIndex=text;task->m12_displayCounter=duration;
    task->m14_cutsceneFilenameRaw=filename.m_offset;task->m18_continuationFlag=-1;
    task->m1A_pad=0;task->m1C_dataPtr={};task->m20_cutsceneTask=nullptr;
    task->m24_savedCounter=0;task->m26_pad=0;
    formationIntroEffect_ResetFrameCounters(task);
    formationIntroEffect_SetupString(task);
    return task;
}

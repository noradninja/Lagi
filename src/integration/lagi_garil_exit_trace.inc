// Bounded observations only: no target eligibility, script or exit-state changes.
static void lagiTraceGarilMainExit(sA5ExitTriggerEntity* self)
{
    const bool scriptActive=isScriptActive()!=0;
    const bool changed=self->lagiTraceLastMode!=self->m32_mode || self->lagiTraceLastScript!=int(scriptActive);
    self->lagiTraceLastMode=self->m32_mode;self->lagiTraceLastScript=scriptActive;
    ++self->lagiTraceTicks;
    if(self->lagiTraceReports>=64 || (!changed && (!self->m68_isVisible || self->lagiTraceTicks%120)))return;
    ++self->lagiTraceReports;
    auto* sub=getFieldTaskPtr()->m8_pSubFieldData;
    auto* lcs=sub->m340_pLCS;
    bool registered=false;
    if(lcs)for(unsigned i=0;i<lcs->mC&&i<0x100;++i)
        registered|=static_cast<const void*>(lcs->m14[i].m0)==static_cast<const void*>(&self->m34_modelCtx);
    // Existing context tail is the native view position + screen-coordinate
    // storage used by LCS. Copy bytes instead of aliasing unrelated C++ types.
    sVec3_FP view; sVec2_S16 screen;
    std::memcpy(&view,self->m34_modelCtx.m24_pad,sizeof(view));
    std::memcpy(&screen,self->m34_modelCtx.m24_pad+sizeof(view),sizeof(screen));
    lagi::platform::logging::writef("[GarilMainExit] mode=%u scriptActive=%d script=%08X discovered=%d visible=%u targetFlags=%04X disable=%02X registered=%d selected=%d lcsCount=%u lcsFlags=%08X viewZ=%d range=%d screen=%d,%d\n",
        unsigned(self->m32_mode),int(scriptActive),unsigned(sub->m34C_ptrToE->m4_currentScript.m_offset),
        int((mainGameState.bitField[0x95]&4)!=0),unsigned(self->m68_isVisible),
        unsigned(self->m34_modelCtx.m10_flags),unsigned(self->m34_modelCtx.m18_visibilityFlags),
        int(registered),int(lcs&&static_cast<const void*>(lcs->m818_curr)==static_cast<const void*>(&self->m34_modelCtx)),
        lcs?unsigned(lcs->mC):0,lcs?unsigned(lcs->m8):0,view.m8_Z.asS32(),
        lcs?lcs->m814_LCSTargetMaxDistance.asS32():0,int(screen[0]),int(screen[1]));
}

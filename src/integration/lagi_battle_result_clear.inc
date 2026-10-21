// BTL_A3 0606FCAE -> 0606E9D2/0606E8CA: coordinate record,
// NOT a progressive-text descriptor. +C is legitimately zero.
static void lagiBattleResultClearArea(sSaturnPtr config)
{
    const s32 x=readSaturnS32(config);
    const s32 y=readSaturnS32(config+4);
    setupVDP2StringRendering(x,y,16,2);
    for(s32 column=vdp2StringContext.mC_X;
        column<vdp2StringContext.mC_X+vdp2StringContext.m14_Width;++column)
        for(s32 row=vdp2StringContext.m10_Y;
            row<vdp2StringContext.m10_Y+vdp2StringContext.m18_Height;++row)
            clearVdp2TextAreaSub1(0,column,row);
    vdp2ResetCursor();
    vdp2DebugPrintSetPosition(x,y);
    clearVdp2TextLargeFont();
}

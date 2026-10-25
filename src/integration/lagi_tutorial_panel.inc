// BTL_T0 0605AD40..0605AE72. Original controller owns visibility/handshake.
static void tutorialDisplay_draw(sTutorialDisplayTask* pThis)
{
    auto* parent=pThis->m0_parent;
    pThis->mB8_frame=static_cast<s16>(static_cast<u16>(pThis->mB8_frame)+1u);
    if(pThis->mBC_subState==0 && (parent->m20_flags&2)) {
        parent->m20_flags&=~4u;++pThis->mBC_subState;
    } else if(pThis->mBC_subState==1 && !(parent->m20_flags&2)) {
        parent->m20_flags|=4;--pThis->mBC_subState;
    }
    if(!(parent->m20_flags&1)) return;
    unsigned frame=0;
    if(parent->m20_flags&4) {
        pThis->mB8_frame=static_cast<s16>(static_cast<u16>(pThis->mB8_frame)&0x7FFF);
        frame=(unsigned(pThis->mB8_frame)%8)<4?1:0;
    }
    if(!gCurrentBattleOverlay || dramAllocatorEnd.size()<=14 ||
        !dramAllocatorEnd[14].mC_fileBundle ||
        !dramAllocatorEnd[14].mC_fileBundle->m4_vd1Allocation) return;
    auto& context=graphicEngineStatus.m14_vdp1Context[0];
    if(context.m0_currentVdp1WriteEA==mainContextVdp1[0].end() ||
        !context.m20_pCurrentVdp1Packet) return;
    const auto sprite=readSaturnEA(gCurrentBattleOverlay->getSaturnPtr(0x060AADE8)+frame*4);
    const auto base=dramAllocatorEnd[14].mC_fileBundle->m4_vd1Allocation->m4_vdp1Memory;
    auto& command=*context.m0_currentVdp1WriteEA;
    command={};command.m0_CMDCTRL=0x1000;
    command.m4_CMDPMOD=readSaturnU16(sprite+10);
    command.m6_CMDCOLR=static_cast<u16>(base+readSaturnU16(sprite+6));
    command.m8_CMDSRCA=static_cast<u16>(base+readSaturnU16(sprite+2));
    command.mA_CMDSIZE=readSaturnU16(sprite+8);
    command.mC_CMDXA=static_cast<s16>(parent->mC-192);
    command.mE_CMDYA=static_cast<s16>(parent->mE-144);
    context.m20_pCurrentVdp1Packet->m4_bucketTypes=0;
    context.m20_pCurrentVdp1Packet->m6_vdp1EA=&command;
    ++context.m20_pCurrentVdp1Packet;++context.m0_currentVdp1WriteEA;
    ++context.m1C;++context.mC;
}

// T0 06054260..060544A0, native tiled/rolled background quad packets.
static void lagiTutorialWaterDraw(sBTL_T0_WaterEffect* self)
{
    if(self->m1C_flag!=1 || self->m18_height2<=0 || !gBattleManager ||
        !gBattleManager->m10_battleOverlay || !gBattleManager->m10_battleOverlay->m8_gridTask) return;
    const auto& clip=graphicEngineStatus.m405C;
    const auto roll=(gBattleManager->m10_battleOverlay->m8_gridTask->m64_cameraRotationTarget[2].asS32()>>16)&0xFFF;
    const s32 sine=getSin(roll).asS32(),cosine=getCos(roll).asS32();
    const s32 half=static_cast<s16>(clip.VDP1_X2)>>1;
    const s32 horizon=self->mC_offsetY;
    const auto fp=[](s32 value){return static_cast<s32>(static_cast<u32>(value)<<16);};
    const auto add=[](s32 a,s32 b){return static_cast<s32>(static_cast<u32>(a)+static_cast<u32>(b));};
    const auto sub=[](s32 a,s32 b){return static_cast<s32>(static_cast<u32>(a)-static_cast<u32>(b));};
    const s32 centerY=fp(horizon);
    for(s32 left=sub(self->m8_offsetX,half);left<=half;left=add(left,self->m18_height2)) {
        const s32 right=add(left,self->m18_height2),top=add(horizon,self->m1A_height3);
        const std::array<std::array<s32,2>,4> points={{{left,top},{right,top},{right,horizon},{left,horizon}}};
        std::array<std::array<s16,2>,4> screen{};
        for(unsigned i=0;i<4;++i) {
            const s32 x=fp(points[i][0]),y=sub(fp(points[i][1]),centerY);
            screen[i][0]=static_cast<s16>(sub(MTH_Mul(fixedPoint(x),fixedPoint(cosine)).asS32(),
                MTH_Mul(fixedPoint(y),fixedPoint(sine)).asS32())>>16);
            const s32 rotatedY=add(sub(0,MTH_Mul(fixedPoint(x),fixedPoint(sine)).asS32()),
                MTH_Mul(fixedPoint(y),fixedPoint(cosine)).asS32());
            screen[i][1]=static_cast<s16>(add(rotatedY,centerY)>>16);
        }
        auto& context=graphicEngineStatus.m14_vdp1Context[0];
        if(context.m0_currentVdp1WriteEA==mainContextVdp1[0].end() || !context.m20_pCurrentVdp1Packet) return;
        auto& cmd=*context.m0_currentVdp1WriteEA;
        cmd={};cmd.m0_CMDCTRL=0x1002;
        cmd.m4_CMDPMOD=static_cast<u16>(0x480|self->m16_height);
        cmd.m6_CMDCOLR=self->m14_width;cmd.m8_CMDSRCA=self->m10_vdp1Base;cmd.mA_CMDSIZE=self->m12;
        cmd.mC_CMDXA=screen[0][0];cmd.mE_CMDYA=static_cast<s16>(-screen[0][1]);
        cmd.m10_CMDXB=screen[1][0];cmd.m12_CMDYB=static_cast<s16>(-screen[1][1]);
        cmd.m14_CMDXC=screen[2][0];cmd.m16_CMDYC=static_cast<s16>(-screen[2][1]);
        cmd.m18_CMDXD=screen[3][0];cmd.m1A_CMDYD=static_cast<s16>(-screen[3][1]);
        const u32 depth=static_cast<u32>(clip.m14_farClipDistance.asS32())-1u;
        context.m20_pCurrentVdp1Packet->m4_bucketTypes=static_cast<u16>(
            (depth*static_cast<u32>(clip.m38_oneOverFarClip.asS32()))>>16);
        context.m20_pCurrentVdp1Packet->m6_vdp1EA=&cmd;
        ++context.m20_pCurrentVdp1Packet;++context.m0_currentVdp1WriteEA;++context.m1C;++context.mC;
    }
}

// T0 06054156..06054216, using the shared native fixed-point services.
static void lagiTutorialWaterUpdate(sBTL_T0_WaterEffect* pThis)
{
    const s32 angle=atan2(cameraProperties2.m0_position[1]>>2,0x80000);
    const s16 combined=static_cast<s16>(cameraProperties2.mC_rotation[0]+angle);
    const fixedPoint distance=FP_Div(0x200000,getCos(static_cast<u16>(angle)&0xFFF));
    const fixedPoint vertical=MTH_Mul(distance,getSin(static_cast<u16>(combined)&0xFFF));
    const fixedPoint depth=MTH_Mul(distance,getCos(static_cast<u16>(combined)&0xFFF));
    pThis->mC_offsetY=static_cast<s32>(4u-static_cast<u32>(setDividend(
        graphicEngineStatus.m405C.m1C_heightScale,vertical,depth)));
    const fixedPoint yaw=MTH_Mul_5_6(fixedPoint(static_cast<s32>(cameraProperties2.mC_rotation[1])),0x3243F,0x200000);
    const fixedPoint horizontal=FP_Div(yaw,0x800);
    const s32 projected=setDividend(graphicEngineStatus.m405C.m18_widthScale,horizontal,0x200000);
    // 06035488 uses DIV0S and sign correction. The reconstructed shared
    // performModulo is unsigned; preserve native signed remainder here.
    pThis->m8_offsetX=pThis->m18_height2-(projected%pThis->m18_height2);
}

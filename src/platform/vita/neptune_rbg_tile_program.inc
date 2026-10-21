static SceGxmFragmentProgram* g_rbgTileProgram=nullptr;
static SceGxmShaderPatcherId g_rbgTileId{};
static bool g_rbgTileRegistered=false,g_rbgTileBound=false,g_rbgTileAttempted=false;
static const SceGxmProgramParameter *g_rbgGenericBindings[22]{},*g_rbgTileBindings[22]{};
static void releaseRbgTileProgram() {
    if(g_probeShaderPatcher && g_rbgTileProgram)
        sceGxmShaderPatcherReleaseFragmentProgram(g_probeShaderPatcher,g_rbgTileProgram);
    if(g_probeShaderPatcher && g_rbgTileRegistered)
        sceGxmShaderPatcherUnregisterProgram(g_probeShaderPatcher,g_rbgTileId);
    g_rbgTileProgram=nullptr;g_rbgTileRegistered=false;
    g_rbgTileBound=false;g_rbgTileAttempted=false;
}
static void selectRbgTileProgram(bool eligible) {
    const SceGxmProgramParameter** slots[22]={
        &g_vdp2Rbg0PlaneParam[0],&g_vdp2Rbg0PlaneParam[1],&g_vdp2Rbg0PlaneParam[2],&g_vdp2Rbg0PlaneParam[3],
        &g_vdp2Rbg0BParam[0],&g_vdp2Rbg0BParam[1],&g_vdp2Rbg0BParam[2],&g_vdp2Rbg0BParam[3],
        &g_vdp2Rbg0BParam[4],&g_vdp2Rbg0BParam[5],&g_vdp2Rbg0BParam[6],&g_vdp2Rbg0BParam[7],&g_vdp2Rbg0BParam[8],
        &g_vdp2Rbg0Transform0Param,&g_vdp2Rbg0Transform1Param,&g_vdp2Rbg0CoefficientParam,
        &g_vdp2Rbg0InfoParam,&g_vdp2Rbg0FormatParam,&g_vdp2Rbg0SwitchCoefficientParam,&g_vdp2Rbg0SwitchInfoParam,
        &g_vdp2Rbg0LayerWindowParam,&g_vdp2Rbg0LayerWindowInfoParam};
    if(eligible && !g_rbgTileAttempted) {
        g_rbgTileAttempted=true;
        for(unsigned i=0;i<22;++i)g_rbgGenericBindings[i]=*slots[i];
        const auto* fp=reinterpret_cast<const SceGxmProgram*>(_binary_lagi_vdp2_rbg0_tile16_8bit_f_gxp_start);
        const auto* vp=reinterpret_cast<const SceGxmProgram*>(_binary_lagi_texture_v_gxp_start);
        const char* names[22]={"rbg0Plane0","rbg0Plane1","rbg0Plane2","rbg0Plane3",
            "rbg0BPlane0","rbg0BPlane1","rbg0BPlane2","rbg0BPlane3","rbg0BTransform0","rbg0BTransform1",
            "rbg0BCoefficient","rbg0BInfo","rbg0BFormat","rbg0Transform0","rbg0Transform1",
            "rbg0Coefficient","rbg0Info","rbg0Format","rbg0SwitchCoefficient","rbg0SwitchInfo",
            "rbg0LayerWindow","rbg0LayerWindowInfo"};
        bool valid=sceGxmProgramCheck(fp)>=0;
        for(unsigned i=0;i<22;++i){g_rbgTileBindings[i]=sceGxmProgramFindParameterByName(fp,names[i]);valid&=g_rbgTileBindings[i]!=nullptr;}
        if(valid && sceGxmShaderPatcherRegisterProgram(g_probeShaderPatcher,fp,&g_rbgTileId)>=0) {
            g_rbgTileRegistered=true;
            SceGxmBlendInfo blend{};
            blend.colorFunc=blend.alphaFunc=SCE_GXM_BLEND_FUNC_ADD;
            blend.colorSrc=blend.alphaSrc=SCE_GXM_BLEND_FACTOR_SRC_ALPHA;
            blend.colorDst=blend.alphaDst=SCE_GXM_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            blend.alphaSrc=SCE_GXM_BLEND_FACTOR_ONE;
            blend.colorMask=SCE_GXM_COLOR_MASK_ALL;
            if(sceGxmShaderPatcherCreateFragmentProgram(g_probeShaderPatcher,g_rbgTileId,
                SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4,SCE_GXM_MULTISAMPLE_NONE,&blend,vp,&g_rbgTileProgram)<0)
                g_rbgTileProgram=nullptr;
        }
        logging::writef("[RBG0TileFormat] specialized8bit16=%u genericFallback=%u\n",g_rbgTileProgram?1u:0u,g_rbgTileProgram?0u:1u);
    }
    const bool use=eligible && g_rbgTileProgram;
    if(use!=g_rbgTileBound) {
        for(unsigned i=0;i<22;++i)*slots[i]=use?g_rbgTileBindings[i]:g_rbgGenericBindings[i];
        g_rbgTileBound=use;
    }
    sceGxmSetFragmentProgram(g_probeContext,use?g_rbgTileProgram:g_vdp2Rbg0FragmentProgram);
}

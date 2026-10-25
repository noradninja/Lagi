static SceGxmFragmentProgram* g_rbgPrograms[3]{};
static SceGxmShaderPatcherId g_rbgIds[3]{};
static bool g_rbgRegistered[3]{},g_rbgAttempted[3]{},g_rbgGenericSaved=false;
static bool g_rbgTileBound=false,g_rbgDecodedBound=false;
static bool g_rbgRowsBound=false;
static int g_rbgBoundVariant=-1;
static const SceGxmProgramParameter *g_rbgGenericBindings[24]{},*g_rbgBindings[3][24]{};
static void releaseRbgTileProgram() {
    for(unsigned v=0;v<3;++v) {
        if(g_probeShaderPatcher && g_rbgPrograms[v])sceGxmShaderPatcherReleaseFragmentProgram(g_probeShaderPatcher,g_rbgPrograms[v]);
        if(g_probeShaderPatcher && g_rbgRegistered[v])sceGxmShaderPatcherUnregisterProgram(g_probeShaderPatcher,g_rbgIds[v]);
        g_rbgPrograms[v]=nullptr;g_rbgRegistered[v]=g_rbgAttempted[v]=false;
    }
    g_rbgRowsBound=g_rbgTileBound=g_rbgDecodedBound=g_rbgGenericSaved=false;g_rbgBoundVariant=-1;
}
static void selectRbgTileProgram(bool eligible,bool decoded=false,bool rows=false) {
    const SceGxmProgramParameter** slots[24]={
        &g_vdp2Rbg0PlaneParam[0],&g_vdp2Rbg0PlaneParam[1],&g_vdp2Rbg0PlaneParam[2],&g_vdp2Rbg0PlaneParam[3],
        &g_vdp2Rbg0BParam[0],&g_vdp2Rbg0BParam[1],&g_vdp2Rbg0BParam[2],&g_vdp2Rbg0BParam[3],
        &g_vdp2Rbg0BParam[4],&g_vdp2Rbg0BParam[5],&g_vdp2Rbg0BParam[6],&g_vdp2Rbg0BParam[7],&g_vdp2Rbg0BParam[8],
        &g_vdp2Rbg0Transform0Param,&g_vdp2Rbg0Transform1Param,&g_vdp2Rbg0CoefficientParam,
        &g_vdp2Rbg0InfoParam,&g_vdp2Rbg0FormatParam,&g_vdp2Rbg0SwitchCoefficientParam,&g_vdp2Rbg0SwitchInfoParam,
        &g_vdp2Rbg0LayerWindowParam,&g_vdp2Rbg0LayerWindowInfoParam,
        &g_vdp2Rbg0PatternBanksParam,&g_vdp2Rbg0CharacterBanksParam};
    if(!g_rbgGenericSaved) {
        for(unsigned i=0;i<24;++i)g_rbgGenericBindings[i]=*slots[i];
        g_rbgGenericSaved=true;
    }
    const unsigned v=decoded?(rows?2u:1u):0u;
    if(eligible && !g_rbgAttempted[v]) {
        g_rbgAttempted[v]=true;
        const auto* fp=reinterpret_cast<const SceGxmProgram*>(decoded && rows?
            _binary_lagi_vdp2_rbg0_rows_f_gxp_start:decoded?
            _binary_lagi_vdp2_rbg0_pages_f_gxp_start:_binary_lagi_vdp2_rbg0_tile16_8bit_f_gxp_start);
        const auto* vp=reinterpret_cast<const SceGxmProgram*>(_binary_lagi_texture_v_gxp_start);
        const char* names[24]={"rbg0Plane0","rbg0Plane1","rbg0Plane2","rbg0Plane3",
            "rbg0BPlane0","rbg0BPlane1","rbg0BPlane2","rbg0BPlane3","rbg0BTransform0","rbg0BTransform1",
            "rbg0BCoefficient","rbg0BInfo","rbg0BFormat","rbg0Transform0","rbg0Transform1",
            "rbg0Coefficient","rbg0Info","rbg0Format","rbg0SwitchCoefficient","rbg0SwitchInfo",
            "rbg0LayerWindow","rbg0LayerWindowInfo","rbg0PatternBanks","rbg0CharacterBanks"};
        bool valid=sceGxmProgramCheck(fp)>=0;
        for(unsigned i=0;i<24;++i){
            g_rbgBindings[v][i]=sceGxmProgramFindParameterByName(fp,names[i]);
            // Rows remove the A-selector accumulator; coefficient.w still
            // carries each parameter's live OVPNR register.
            if(!(v==2 && i==18) && !(decoded && i==22))valid&=g_rbgBindings[v][i]!=nullptr;
        }
        if(valid && sceGxmShaderPatcherRegisterProgram(g_probeShaderPatcher,fp,&g_rbgIds[v])>=0) {
            g_rbgRegistered[v]=true;
            SceGxmBlendInfo blend{};
            blend.colorFunc=blend.alphaFunc=SCE_GXM_BLEND_FUNC_ADD;
            blend.colorSrc=blend.alphaSrc=SCE_GXM_BLEND_FACTOR_SRC_ALPHA;
            blend.colorDst=blend.alphaDst=SCE_GXM_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            blend.alphaSrc=SCE_GXM_BLEND_FACTOR_ONE;
            blend.colorMask=SCE_GXM_COLOR_MASK_ALL;
            if(sceGxmShaderPatcherCreateFragmentProgram(g_probeShaderPatcher,g_rbgIds[v],
                SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4,SCE_GXM_MULTISAMPLE_NONE,&blend,vp,&g_rbgPrograms[v])<0)
                g_rbgPrograms[v]=nullptr;
        }
        logging::writef("[RBG0TileFormat] specialized8bit16=%u decodedPages=%u genericFallback=%u\n",g_rbgPrograms[v]?1u:0u,decoded?1u:0u,g_rbgPrograms[v]?0u:1u);
    }
    if(eligible && v==2 && !g_rbgPrograms[v]) {
        selectRbgTileProgram(eligible,true,false);
        return;
    }
    const int use=eligible && g_rbgPrograms[v]?static_cast<int>(v):-1;
    if(use!=g_rbgBoundVariant) {
        for(unsigned i=0;i<24;++i)*slots[i]=use>=0?g_rbgBindings[use][i]:g_rbgGenericBindings[i];
        g_rbgBoundVariant=use;
    }
    g_rbgTileBound=use==0;g_rbgDecodedBound=use==1 || use==2;g_rbgRowsBound=use==2;
    sceGxmSetFragmentProgram(g_probeContext,use>=0?g_rbgPrograms[use]:g_vdp2Rbg0FragmentProgram);
}

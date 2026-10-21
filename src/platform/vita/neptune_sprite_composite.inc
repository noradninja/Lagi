// The first supported native route is type-3 sprite selection over RBG0/BACK.
// Unsupported additional VDP2 layers/windows stay on the established route.
static void* g_spriteLayerPixels=nullptr;
static SceUID g_spriteLayerUid=-1;
static SceGxmColorSurface g_spriteLayerSurface{};
static SceGxmTexture g_spriteLayerTexture{};
static SceGxmFragmentProgram* g_spriteCompositeProgram=nullptr;
static SceGxmShaderPatcherId g_spriteCompositeId{};
static bool g_spriteCompositeRegistered=false;
static void* g_spriteLookupPixels=nullptr;
static SceUID g_spriteLookupUid=-1;
static SceGxmTexture g_spriteLookupTexture{};
static const SceGxmProgramParameter *g_spriteGeometry=nullptr,*g_spriteBackColor=nullptr;
static void releaseSpriteComposite() {
    freeMovieMappedBlock(g_spriteLayerUid,g_spriteLayerPixels);
    freeMovieMappedBlock(g_spriteLookupUid,g_spriteLookupPixels);
    if(g_probeShaderPatcher && g_spriteCompositeProgram)
        sceGxmShaderPatcherReleaseFragmentProgram(g_probeShaderPatcher,g_spriteCompositeProgram);
    g_spriteCompositeProgram=nullptr;
    if(g_probeShaderPatcher && g_spriteCompositeRegistered)
        sceGxmShaderPatcherUnregisterProgram(g_probeShaderPatcher,g_spriteCompositeId);
    g_spriteCompositeRegistered=false;
}
static bool prepareSpriteComposite(bool native,bool menu) {
    const bool supported=LAGI_VDP2_SPRITE_COMPOSITE && native && !menu && !g_halfResolution &&
        (g_spriteColor.ccctl&0x40) && (g_spriteColor.spctl&15)==3 &&
        !g_spriteColor.colorWindow && !(g_spriteColor.ccctl&0x610) &&
        !(g_spriteColor.bgon&0x5) && (g_spriteColor.spctl&0x20) &&
        g_sceneVdp2Vertices && g_sceneVdp2Indices;
    static std::uint64_t last=~std::uint64_t(0);
    static unsigned budget=32;
    const std::uint64_t key=std::uint64_t(g_spriteColor.ccctl) |
        (std::uint64_t(g_spriteColor.spctl)<<16) |
        (std::uint64_t(g_spriteColor.bgon)<<32) | (std::uint64_t(supported)<<63);
    if(LAGI_VDP2_SPRITE_COMPOSITE && budget && key!=last) {
        --budget;last=key;
        logging::writef("[VDP2SpriteComposite] active=%u CCCTL=%04X SPCTL=%04X BGON=%04X colorWindow=%02X menu=%u nativeFull=%u\n",
            supported?1u:0u,g_spriteColor.ccctl,g_spriteColor.spctl,g_spriteColor.bgon,
            g_spriteColor.colorWindow,menu?1u:0u,native?1u:0u);
    }
    if(!supported)return false;
    // Destination-alpha mode and extra world layers need a multi-layer
    // top/second-source resolver; they must not silently use this route.
    if(!g_spriteCompositeProgram) {
        const auto* fp=reinterpret_cast<const SceGxmProgram*>(_binary_lagi_sprite_composite_f_gxp_start);
        const auto* vp=reinterpret_cast<const SceGxmProgram*>(_binary_lagi_texture_v_gxp_start);
        if(sceGxmProgramCheck(fp)<0)return false;
        if(!g_spriteCompositeRegistered) {
            if(sceGxmShaderPatcherRegisterProgram(g_probeShaderPatcher,fp,&g_spriteCompositeId)<0)return false;
            g_spriteCompositeRegistered=true;
        }
        if(sceGxmShaderPatcherCreateFragmentProgram(g_probeShaderPatcher,g_spriteCompositeId,
            SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4,SCE_GXM_MULTISAMPLE_NONE,nullptr,vp,
            &g_spriteCompositeProgram)<0)return false;
        g_spriteGeometry=sceGxmProgramFindParameterByName(fp,"geometry");
        g_spriteBackColor=sceGxmProgramFindParameterByName(fp,"backColor");
        if(!g_spriteGeometry||!g_spriteBackColor) {
            releaseSpriteComposite();return false;
        }
    }
    if(!g_spriteLayerPixels) {
        g_spriteLayerPixels=probeGpuAlloc(kGxmPitch*kHeight*4,
            static_cast<SceGxmMemoryAttribFlags>(SCE_GXM_MEMORY_ATTRIB_READ|SCE_GXM_MEMORY_ATTRIB_WRITE),
            &g_spriteLayerUid);
        if(!g_spriteLayerPixels)return false;
        if(sceGxmColorSurfaceInit(&g_spriteLayerSurface,SCE_GXM_COLOR_FORMAT_A8B8G8R8,
            SCE_GXM_COLOR_SURFACE_LINEAR,SCE_GXM_COLOR_SURFACE_SCALE_NONE,
            SCE_GXM_OUTPUT_REGISTER_SIZE_32BIT,kWidth,kHeight,kGxmPitch,g_spriteLayerPixels)<0 ||
            sceGxmTextureInitLinear(&g_spriteLayerTexture,g_spriteLayerPixels,
                SCE_GXM_TEXTURE_FORMAT_A8B8G8R8,kGxmPitch,kHeight,0)<0) {
            freeMovieMappedBlock(g_spriteLayerUid,g_spriteLayerPixels);return false;
        }
        sceGxmTextureSetMinFilter(&g_spriteLayerTexture,SCE_GXM_TEXTURE_FILTER_POINT);
        sceGxmTextureSetMagFilter(&g_spriteLayerTexture,SCE_GXM_TEXTURE_FILTER_POINT);
        logging::writef("[VDP2SpriteComposite] experimental type3 RBG0/BACK world pass ready; NBG1/3 UI retained separately\n");
    }
    if(!g_spriteLookupPixels) {
        g_spriteLookupPixels=probeGpuAlloc(256*4,
            static_cast<SceGxmMemoryAttribFlags>(SCE_GXM_MEMORY_ATTRIB_READ|SCE_GXM_MEMORY_ATTRIB_WRITE),
            &g_spriteLookupUid);
        if(!g_spriteLookupPixels)return false;
        if(sceGxmTextureInitLinear(&g_spriteLookupTexture,g_spriteLookupPixels,
            SCE_GXM_TEXTURE_FORMAT_A8B8G8R8,256,1,0)<0) {
            freeMovieMappedBlock(g_spriteLookupUid,g_spriteLookupPixels);return false;
        }
        sceGxmTextureSetMinFilter(&g_spriteLookupTexture,SCE_GXM_TEXTURE_FILTER_POINT);
        sceGxmTextureSetMagFilter(&g_spriteLookupTexture,SCE_GXM_TEXTURE_FILTER_POINT);
        logging::writef("[VDP2SpriteComposite] register decision lookup ready (256 entries)\n");
    }
    // Previous frame completes before this preparation; no in-flight texture
    // is mutated. This table contains only published register decisions.
    auto* lookup=static_cast<std::uint32_t*>(g_spriteLookupPixels);
    for(unsigned i=0;i<256;++i)lookup[i]=lagi::spriteCompositeLookup(g_spriteColor,i);
    return clearNativeSceneColorBuffer(static_cast<std::uint32_t*>(g_spriteLayerPixels),kWidth,kHeight,kGxmPitch);
}
static bool finishSpriteComposite(SceGxmRenderTarget* target,SceGxmColorSurface* destination,
    SceGxmSyncObject* sync) {
    sceGxmEndScene(g_probeContext,nullptr,nullptr);
    if(sceGxmBeginScene(g_probeContext,0,target,nullptr,nullptr,sync,destination,nullptr)<0)return false;
    sceGxmSetVertexProgram(g_probeContext,g_textureVertexProgram);
    sceGxmSetFragmentProgram(g_probeContext,g_spriteCompositeProgram);
    sceGxmSetCullMode(g_probeContext,SCE_GXM_CULL_NONE);
    sceGxmSetFrontDepthFunc(g_probeContext,SCE_GXM_DEPTH_FUNC_ALWAYS);
    sceGxmSetBackDepthFunc(g_probeContext,SCE_GXM_DEPTH_FUNC_ALWAYS);
    sceGxmSetFrontDepthWriteEnable(g_probeContext,SCE_GXM_DEPTH_WRITE_DISABLED);
    sceGxmSetBackDepthWriteEnable(g_probeContext,SCE_GXM_DEPTH_WRITE_DISABLED);
    sceGxmSetFrontPolygonMode(g_probeContext,SCE_GXM_POLYGON_MODE_TRIANGLE_FILL);
    sceGxmSetBackPolygonMode(g_probeContext,SCE_GXM_POLYGON_MODE_TRIANGLE_FILL);
    sceGxmSetDefaultRegionClipAndViewport(g_probeContext,kWidth-1,kHeight-1);
    void *v=nullptr,*f=nullptr;
    if(sceGxmReserveVertexDefaultUniformBuffer(g_probeContext,&v)<0 || !v ||
        sceGxmReserveFragmentDefaultUniformBuffer(g_probeContext,&f)<0 || !f)return false;
    const auto identity=viewerIdentity();
    sceGxmSetUniformDataF(v,g_textureWvpParam,0,16,identity.m);
    const float geometry[4]={float(kWidth)/kGxmPitch,1,g_sceneRbg0Enabled?1.f:0.f,float(g_spriteColor.prir&7)};
    const float back[4]={0,0,0,1}; // Matches clearNativeSceneColorBuffer.
    sceGxmSetUniformDataF(f,g_spriteGeometry,0,4,geometry);
    sceGxmSetUniformDataF(f,g_spriteBackColor,0,4,back);
    sceGxmSetFragmentTexture(g_probeContext,0,&g_spriteLayerTexture);
    sceGxmSetFragmentTexture(g_probeContext,1,g_sceneRbg0Enabled?&g_sceneRbg0ResolvedTexture:&g_spriteLayerTexture);
    sceGxmSetFragmentTexture(g_probeContext,2,&g_spriteLookupTexture);
    sceGxmSetVertexStream(g_probeContext,0,g_sceneVdp2Vertices);
    return sceGxmDraw(g_probeContext,SCE_GXM_PRIMITIVE_TRIANGLES,SCE_GXM_INDEX_FORMAT_U16,g_sceneVdp2Indices,6)>=0;
}

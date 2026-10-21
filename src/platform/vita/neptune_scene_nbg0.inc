// Shared raw-VRAM upload and native-raster GPU resolve. No CPU pixel decoder,
// stochastic sampling or effect-specific placement is used for this layer.
static bool renderSceneNbg0Gpu()
{
    if(!g_sceneNbg0.enabled || !g_sceneNbg0.gpu[7][2])return true;
    if(!g_sceneVdp2GpuRaw || !g_sceneRbg0RenderTarget || !g_sceneNbg0Param)return false;
    if(!g_nbg0RowsPixels){
        g_nbg0RowsPixels=probeGpuAlloc(sizeof(lagi::Nbg0LineRows),SCE_GXM_MEMORY_ATTRIB_READ,&g_nbg0RowsUid);
        if(!g_nbg0RowsPixels || sceGxmTextureInitLinear(&g_nbg0RowsTexture,g_nbg0RowsPixels,
            SCE_GXM_TEXTURE_FORMAT_F32F32_GR,256,2,0)<0){
            freeMovieMappedBlock(g_nbg0RowsUid,g_nbg0RowsPixels);return false;
        }
        sceGxmTextureSetMinFilter(&g_nbg0RowsTexture,SCE_GXM_TEXTURE_FILTER_POINT);
        sceGxmTextureSetMagFilter(&g_nbg0RowsTexture,SCE_GXM_TEXTURE_FILTER_POINT);
    }
    // Same native table, evaluated once per scanline, not per cloud pixel.
    const auto lineRows=lagi::buildNbg0LineRows(g_sceneVdp2Raw,g_sceneNbg0.gpu);
    std::memcpy(g_nbg0RowsPixels,lineRows.data(),sizeof(lineRows));
    if(!g_sceneNbg0Pixels) {
        g_sceneNbg0Pixels=probeCdramAlloc(kSceneRbg0Pitch*kSceneRbg0Height*4,
            SCE_GXM_MEMORY_ATTRIB_READ|SCE_GXM_MEMORY_ATTRIB_WRITE,&g_sceneNbg0Uid);
        if(!g_sceneNbg0Pixels)g_sceneNbg0Pixels=probeGpuAlloc(kSceneRbg0Pitch*kSceneRbg0Height*4,
            SCE_GXM_MEMORY_ATTRIB_READ|SCE_GXM_MEMORY_ATTRIB_WRITE,&g_sceneNbg0Uid);
        if(!g_sceneNbg0Pixels)return false;
        if(sceGxmColorSurfaceInit(&g_sceneNbg0Surface,SCE_GXM_COLOR_FORMAT_A8B8G8R8,
            SCE_GXM_COLOR_SURFACE_LINEAR,SCE_GXM_COLOR_SURFACE_SCALE_NONE,
            SCE_GXM_OUTPUT_REGISTER_SIZE_32BIT,kSceneRbg0Width,kSceneRbg0Height,
            kSceneRbg0Pitch,g_sceneNbg0Pixels)<0 ||
            sceGxmTextureInitLinearStrided(&g_sceneNbg0Texture,g_sceneNbg0Pixels,
            SCE_GXM_TEXTURE_FORMAT_A8B8G8R8,kSceneRbg0Width,kSceneRbg0Height,kSceneRbg0Pitch*4)<0) {
            freeMovieMappedBlock(g_sceneNbg0Uid,g_sceneNbg0Pixels);return false;
        }
        sceGxmTextureSetMinFilter(&g_sceneNbg0Texture,SCE_GXM_TEXTURE_FILTER_LINEAR);
        sceGxmTextureSetMagFilter(&g_sceneNbg0Texture,SCE_GXM_TEXTURE_FILTER_LINEAR);
    }
    std::memset(g_sceneNbg0Pixels,0,kSceneRbg0Pitch*kSceneRbg0Height*4);
    if(sceGxmBeginScene(g_probeContext,0,g_sceneRbg0RenderTarget,nullptr,nullptr,nullptr,
        &g_sceneNbg0Surface,nullptr)<0)return false;
    sceGxmSetDefaultRegionClipAndViewport(g_probeContext,kSceneRbg0Width-1,kSceneRbg0Height-1);
    sceGxmSetVertexProgram(g_probeContext,g_textureVertexProgram);
    sceGxmSetFragmentProgram(g_probeContext,g_sceneNbg0ResolveProgram);
    sceGxmSetFrontDepthFunc(g_probeContext,SCE_GXM_DEPTH_FUNC_ALWAYS);
    sceGxmSetBackDepthFunc(g_probeContext,SCE_GXM_DEPTH_FUNC_ALWAYS);
    sceGxmSetFrontDepthWriteEnable(g_probeContext,SCE_GXM_DEPTH_WRITE_DISABLED);
    sceGxmSetBackDepthWriteEnable(g_probeContext,SCE_GXM_DEPTH_WRITE_DISABLED);
    sceGxmSetCullMode(g_probeContext,SCE_GXM_CULL_NONE);
    sceGxmSetFrontPolygonMode(g_probeContext,SCE_GXM_POLYGON_MODE_TRIANGLE_FILL);
    sceGxmSetBackPolygonMode(g_probeContext,SCE_GXM_POLYGON_MODE_TRIANGLE_FILL);
    void* vertex=nullptr;void* fragment=nullptr;
    bool ok=sceGxmReserveVertexDefaultUniformBuffer(g_probeContext,&vertex)>=0 && vertex &&
        sceGxmReserveFragmentDefaultUniformBuffer(g_probeContext,&fragment)>=0 && fragment;
    if(ok) {
        const auto identity=viewerIdentity();
        sceGxmSetUniformDataF(vertex,g_textureWvpParam,0,16,identity.m);
        sceGxmSetUniformDataF(fragment,g_sceneNbg0Param,0,32,&g_sceneNbg0.gpu[0][0]);
        sceGxmSetFragmentTexture(g_probeContext,0,&g_sceneVdp2Texture);
        sceGxmSetFragmentTexture(g_probeContext,1,&g_nbg0RowsTexture);
        ok=sceGxmSetVertexStream(g_probeContext,0,g_sceneVdp2Vertices)>=0 &&
            sceGxmDraw(g_probeContext,SCE_GXM_PRIMITIVE_TRIANGLES,SCE_GXM_INDEX_FORMAT_U16,g_sceneVdp2Indices,6)>=0;
    }
    sceGxmEndScene(g_probeContext,nullptr,nullptr);
    static unsigned reports=0;
    if(reports++<4)logging::writef("[SceneNBG0] nativeGpu=1 raster=352x224 additive=%u priority=%u ok=%u stochastic=0 lineRows=1 rowBytes=4096\n",
        g_sceneNbg0.additive?1u:0u,unsigned(g_sceneNbg0.gpu[7][2]),ok?1u:0u);
    return ok;
}
static void drawSceneNbg0Gpu()
{
    if(!g_sceneNbg0.enabled || !g_sceneNbg0.gpu[7][2] || !g_sceneNbg0Pixels)return;
    sceGxmSetVertexProgram(g_probeContext,g_textureVertexProgram);
    sceGxmSetFragmentProgram(g_probeContext,g_sceneNbg0.additive?g_sceneNbg0AddProgram:g_sceneNbg0OverProgram);
    sceGxmSetViewport(g_probeContext,viewerRenderWidth()*0.5f,viewerRenderWidth()*0.5f,
        viewerRenderHeight()*0.5f,-viewerRenderHeight()*0.5f,0.5f,0.5f);
    sceGxmSetCullMode(g_probeContext,SCE_GXM_CULL_NONE);
    sceGxmSetFrontPolygonMode(g_probeContext,SCE_GXM_POLYGON_MODE_TRIANGLE_FILL);
    sceGxmSetBackPolygonMode(g_probeContext,SCE_GXM_POLYGON_MODE_TRIANGLE_FILL);
    sceGxmSetFrontDepthFunc(g_probeContext,SCE_GXM_DEPTH_FUNC_ALWAYS);
    sceGxmSetBackDepthFunc(g_probeContext,SCE_GXM_DEPTH_FUNC_ALWAYS);
    sceGxmSetFrontDepthWriteEnable(g_probeContext,SCE_GXM_DEPTH_WRITE_DISABLED);
    sceGxmSetBackDepthWriteEnable(g_probeContext,SCE_GXM_DEPTH_WRITE_DISABLED);
    void* uniforms=nullptr;
    if(sceGxmReserveVertexDefaultUniformBuffer(g_probeContext,&uniforms)<0 || !uniforms)return;
    const auto identity=viewerIdentity();
    sceGxmSetUniformDataF(uniforms,g_textureWvpParam,0,16,identity.m);
    void* fragment=nullptr;
    if(sceGxmReserveFragmentDefaultUniformBuffer(g_probeContext,&fragment)<0 || !fragment)return;
    const float info[4]={1,0,0,0};
    sceGxmSetUniformDataF(fragment,g_rgbaPresentInfo,0,4,info);
    sceGxmSetFragmentTexture(g_probeContext,0,&g_sceneNbg0Texture);
    if(sceGxmSetVertexStream(g_probeContext,0,g_sceneVdp2Vertices)>=0)
        sceGxmDraw(g_probeContext,SCE_GXM_PRIMITIVE_TRIANGLES,SCE_GXM_INDEX_FORMAT_U16,g_sceneVdp2Indices,6);
}

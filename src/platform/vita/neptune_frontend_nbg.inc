// Called inside the native 352x224 resolve, after RBG-only color offset and
// before scaling/text/VDP1 overlays. Decode each native NBG pixel once rather
// than repeating dependent VRAM reads for every enlarged display pixel.
static bool drawFrontendNbgNative() {
    if(g_movieVdp2Info[0]<0.5f || g_movieVdp2Info[3]<0.5f)return true;
    sceGxmSetVertexProgram(g_probeContext,g_textureVertexProgram);
    void* vertexUniforms=nullptr;
    if(sceGxmReserveVertexDefaultUniformBuffer(g_probeContext,&vertexUniforms)<0 || !vertexUniforms)
        return false;
    const ViewerMat4 identity=viewerIdentity();
    sceGxmSetUniformDataF(vertexUniforms,g_textureWvpParam,0,16,identity.m);
    sceGxmSetFragmentTexture(g_probeContext,0,&g_movieTexture);
    sceGxmSetFragmentProgram(g_probeContext,g_vdp2NbgFragmentProgram);
    void* uniforms=nullptr;
    if(sceGxmReserveFragmentDefaultUniformBuffer(g_probeContext,&uniforms)<0 || !uniforms)
        return false;
    sceGxmSetUniformDataF(uniforms,g_vdp2InfoParam,0,4,g_movieVdp2Info);
    if(sceGxmSetVertexStream(g_probeContext,0,g_movieVertices+4)<0)return false;
    return sceGxmDraw(g_probeContext,SCE_GXM_PRIMITIVE_TRIANGLES,
        SCE_GXM_INDEX_FORMAT_U16,g_movieIndices,6u)>=0;
}

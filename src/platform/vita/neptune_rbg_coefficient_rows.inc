// A resource predecode, not a replacement rotation implementation. Every
// coefficient comes from the current published Azel table; GPU projection,
// RPMD selection, clipping, palette and blending remain in the shared shader.
static void* g_rbgCoefficientRowsPixels=nullptr;
static SceUID g_rbgCoefficientRowsUid=-1;
static bool g_rbgCoefficientRowsAttempted=false;
static void releaseRbgCoefficientRows() {
    freeMovieMappedBlock(g_rbgCoefficientRowsUid,g_rbgCoefficientRowsPixels);
    g_rbgCoefficientRowsReady=false;g_rbgCoefficientRowsAttempted=false;
}
static bool prepareRbgCoefficientRows() {
    if(!LAGI_RBG_COEFFICIENT_ROWS || !g_rbgDecodedPagesReady)return false;
    const auto start=sceKernelGetProcessTimeWide();
    static unsigned lastReason=~0u,budget=24;
    auto status=[&](unsigned reason) {
        if(reason!=lastReason && budget) {
            --budget;
            logging::writef("[RBG0CoefficientRows] active=%u reason=%u prepUs=%u bytes=4096\n",
                reason==0?1u:0u,reason,static_cast<unsigned>(sceKernelGetProcessTimeWide()-start));
        }
        lastReason=reason;return reason==0;
    };
    const unsigned mode=static_cast<unsigned>(g_sceneRbg0.ctrl[0])&3;
    const unsigned ktctl=static_cast<unsigned>(g_sceneRbg0.ctrl[1]);
    const unsigned ktaof=static_cast<unsigned>(g_sceneRbg0.ctrl[2]);
    std::array<lagi::RbgCoefficientRows,2> rows{};
    for(unsigned table=0;table<2;++table) {
        const bool used=table?mode!=0:mode!=1;
        const bool enabled=used && (ktctl&(table?0x100:1));
        const unsigned size=(ktctl&(table?0x200:2))?2:4;
        const unsigned base=((ktaof>>(table?8:0))&7)*size*65536;
        if(!lagi::buildRbgCoefficientRows(g_sceneVdp2Raw,
            table?g_sceneRbg0.coefficientB:g_sceneRbg0.coefficientA,
            base,size,enabled,rows[table]))return status(1);
    }
    if(!g_rbgCoefficientRowsAttempted) {
        g_rbgCoefficientRowsAttempted=true;
        g_rbgCoefficientRowsPixels=probeGpuAlloc(sizeof(rows),SCE_GXM_MEMORY_ATTRIB_READ,
            &g_rbgCoefficientRowsUid);
        if(!g_rbgCoefficientRowsPixels || sceGxmTextureInitLinear(&g_rbgCoefficientRowsTexture,
            g_rbgCoefficientRowsPixels,SCE_GXM_TEXTURE_FORMAT_F32F32_GR,256,2,0)<0) {
            releaseRbgCoefficientRows();g_rbgCoefficientRowsAttempted=true;
            return status(2);
        }
        sceGxmTextureSetMinFilter(&g_rbgCoefficientRowsTexture,SCE_GXM_TEXTURE_FILTER_POINT);
        sceGxmTextureSetMagFilter(&g_rbgCoefficientRowsTexture,SCE_GXM_TEXTURE_FILTER_POINT);
    }
    if(!g_rbgCoefficientRowsPixels)return status(2);
    static_assert(sizeof(rows)==4096);
    std::memcpy(g_rbgCoefficientRowsPixels,rows.data(),sizeof(rows));
    return status(0);
}

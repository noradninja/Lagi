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
    // Garil A5's RPMD=2 background is not the A3 repeated VDP1 sprite
    // strip. It selects rotation parameter B (sky) where A's coefficient
    // carries the invalid bit. Trace the authored selection boundary before
    // making changes to its map, window or compositing policy. This uses
    // already-decoded CPU rows: no GPU readback or per-frame work.
    if(mode==2u && g_sceneGameMode==3u) {
        static unsigned a5SelectionReports=0u;
        if(a5SelectionReports<3u) {
            ++a5SelectionReports;
            unsigned aRows=0u,bRows=0u,transitions=0u;
            int firstB=-1,lastB=-1;
            bool previousB=rows[0][0].invalid>=0.5f;
            for(unsigned y=0;y<224u;++y) {
                const bool useB=rows[0][y].invalid>=0.5f;
                if(useB) {
                    ++bRows;
                    if(firstB<0)firstB=int(y);
                    lastB=int(y);
                } else ++aRows;
                if(y && useB!=previousB)++transitions;
                previousB=useB;
            }
            logging::writef(
                "[A5RotationSelection] RPMD=%u KTCTL=%04X KTAOF=%04X Arows=%u Brows=%u changes=%u firstB=%d lastB=%d "
                "y0=%u y32=%u y64=%u y96=%u y112=%u y128=%u y160=%u y192=%u y223=%u "
                "Acoeff0=%.5f Bcoeff0=%.5f\n",
                mode,ktctl,ktaof,aRows,bRows,transitions,firstB,lastB,
                unsigned(rows[0][0].invalid>=0.5f),
                unsigned(rows[0][32].invalid>=0.5f),
                unsigned(rows[0][64].invalid>=0.5f),
                unsigned(rows[0][96].invalid>=0.5f),
                unsigned(rows[0][112].invalid>=0.5f),
                unsigned(rows[0][128].invalid>=0.5f),
                unsigned(rows[0][160].invalid>=0.5f),
                unsigned(rows[0][192].invalid>=0.5f),
                unsigned(rows[0][223].invalid>=0.5f),
                double(rows[0][0].scale),double(rows[1][0].scale));
        }
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

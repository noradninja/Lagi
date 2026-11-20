// The decoded RBG page atlas stores native palette indices, not frozen colors.
// Keep one small GPU-local copy of the currently published 2048-entry CRAM
// palette so per-pixel RGB555 extraction is not repeated in the RBG shader.
static void releaseRbgPaletteLut() {
    freeMovieMappedBlock(g_rbgPaletteLutUid,g_rbgPaletteLutPixels);
    g_rbgPaletteLutPixels=nullptr;
    g_rbgPaletteLutInitialized=false;
}

static bool prepareRbgPaletteLut(const void* rawTextureData) {
    if(!rawTextureData)return false;
    constexpr unsigned kEntries=2048u;
    constexpr unsigned kCramBytes=kEntries*2u;
    const auto* cram=static_cast<const unsigned char*>(rawTextureData)+0x80000u;
    if(!g_rbgPaletteLutPixels) {
        g_rbgPaletteLutPixels=probeCdramAlloc(kEntries*sizeof(std::uint32_t),
            SCE_GXM_MEMORY_ATTRIB_READ,&g_rbgPaletteLutUid);
        const bool cdram=g_rbgPaletteLutPixels!=nullptr;
        if(!g_rbgPaletteLutPixels)
            g_rbgPaletteLutPixels=probeGpuAlloc(kEntries*sizeof(std::uint32_t),
                SCE_GXM_MEMORY_ATTRIB_READ,&g_rbgPaletteLutUid);
        if(!g_rbgPaletteLutPixels)return false;
        if(sceGxmTextureInitLinear(&g_rbgPaletteLutTexture,g_rbgPaletteLutPixels,
                SCE_GXM_TEXTURE_FORMAT_A8B8G8R8,kEntries,1u,0)<0) {
            releaseRbgPaletteLut();
            return false;
        }
        sceGxmTextureSetMinFilter(&g_rbgPaletteLutTexture,SCE_GXM_TEXTURE_FILTER_POINT);
        sceGxmTextureSetMagFilter(&g_rbgPaletteLutTexture,SCE_GXM_TEXTURE_FILTER_POINT);
        logging::writef("[RBG0PaletteLut] active=1 cdram=%u entries=%u\n",
            cdram?1u:0u,kEntries);
    }
    if(!g_rbgPaletteLutInitialized ||
        std::memcmp(g_rbgPaletteLutSource,cram,kCramBytes)!=0) {
        auto* colors=static_cast<std::uint32_t*>(g_rbgPaletteLutPixels);
        for(unsigned i=0;i<kEntries;++i)
            colors[i]=vdp2Rgb555ToAbgr(readVdp2Be16(cram,i*2u));
        std::memcpy(g_rbgPaletteLutSource,cram,kCramBytes);
        g_rbgPaletteLutInitialized=true;
    }
    return true;
}

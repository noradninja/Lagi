// Bounded resource cache, never a scene-specific water shortcut. Layouts
// exceeding capacity retain the complete raw decoder rather than dropping pages.
static constexpr unsigned kRbgPageSlots=4,kRbgAtlasWidth=1024;
static void* g_rbgAtlasPixels=nullptr;
static std::array<unsigned,256> g_rbgPageLookup{};
static SceUID g_rbgAtlasUid=-1;
static bool g_rbgPageAllocationAttempted=false;
static std::array<lagi::RbgDecodedPage,kRbgPageSlots> g_rbgPages{};
static std::array<bool,kRbgPageSlots> g_rbgPageAssigned{};
static std::uint32_t* g_rbgPageScratch=nullptr;
static bool g_rbgPageFrontend=false;
static void releaseRbgDecodedPages() {
    freeMovieMappedBlock(g_rbgAtlasUid,g_rbgAtlasPixels);
    g_rbgDecodedPagesReady=false;g_rbgPageAllocationAttempted=false;
    g_rbgPageAssigned.fill(false);
    for(auto& page:g_rbgPages)page.reset();
    std::free(g_rbgPageScratch);g_rbgPageScratch=nullptr;
}
template<class State> static bool prepareRbgDecodedPagesFor(const State& rbg,
    const unsigned char* raw,const std::uint32_t* sourceGenerations,bool frontend) {
    if(!LAGI_RBG_DECODED_PAGES)return false;
    // Independent producer generation counters can have equal numeric values.
    // Never accept one domain's resource as a hit in the other domain.
    if(frontend!=g_rbgPageFrontend) {
        g_rbgPageAssigned.fill(false);
        for(auto& page:g_rbgPages)page.reset();
        g_rbgPageFrontend=frontend;
    }
    const auto start=sceKernelGetProcessTimeWide();
    unsigned decodeUs=0,atlasCopyUs=0;
    static unsigned frame=0,lastReason=~0u,logBudget=32;
    ++frame;
    auto status=[&](unsigned reason,unsigned pages,unsigned rebuilt,unsigned hits) {
        if((reason!=lastReason && logBudget) || (frame%120==0) || (rebuilt && logBudget)) {
            if(logBudget)--logBudget;
            logging::writef("[RBG0PageCache] active=%u reason=%u pages=%u rebuilt=%u hits=%u prepUs=%u uploadBytes=%u frontend=%u decodeUs=%u atlasCopyUs=%u\n",
                reason==0?1u:0u,reason,pages,rebuilt,hits,
                static_cast<unsigned>(sceKernelGetProcessTimeWide()-start),rebuilt*512u*512u*4u,frontend?1u:0u,
                decodeUs,atlasCopyUs);
        }
        lastReason=reason;
        return reason==0;
    };
    if(!lagi::rbgTile16EightBit(static_cast<unsigned>(rbg.format[0]),
        static_cast<unsigned>(rbg.format[1])))return status(1,0,0,0);
    std::array<unsigned,kRbgPageSlots> required{};
    unsigned count=0;
    auto collect=[&](const float* planes,unsigned plsz) {
        const unsigned pageWidth=plsz>=1?2:1,pageHeight=plsz>=2?2:1;
        for(unsigned plane=0;plane<16;++plane)for(unsigned y=0;y<pageHeight;++y)
            for(unsigned x=0;x<pageWidth;++x) {
                const unsigned base=static_cast<unsigned>(planes[plane])+(y*pageWidth+x)*2048;
                if((base&2047) || base>0x80000-2048)return false;
                bool found=false;for(unsigned i=0;i<count;++i)found|=required[i]==base;
                if(!found) {if(count==required.size())return false;required[count++]=base;}
            }
        return true;
    };
    const unsigned mode=static_cast<unsigned>(rbg.ctrl[0])&3;
    const auto overMode=[](float format){return (static_cast<unsigned>(format)>>2)&3;};
    if((mode!=1 && overMode(rbg.parameterFormat[0])==1) ||
       (mode!=0 && overMode(rbg.parameterFormat[1])==1))return status(5,0,0,0);
    if((mode!=1 && !collect(rbg.planesA,static_cast<unsigned>(rbg.parameterFormat[0])&3)) ||
       (mode!=0 && !collect(rbg.planesB,static_cast<unsigned>(rbg.parameterFormat[1])&3)))
        return status(2,count,0,0);
    if(!g_rbgPageAllocationAttempted) {
        g_rbgPageAllocationAttempted=true;
        g_rbgAtlasPixels=probeCdramAlloc(kRbgAtlasWidth*kRbgAtlasWidth*4,
            SCE_GXM_MEMORY_ATTRIB_READ,&g_rbgAtlasUid);
        if(!g_rbgAtlasPixels ||
            sceGxmTextureInitLinear(&g_rbgDecodedAtlasTexture,g_rbgAtlasPixels,
                SCE_GXM_TEXTURE_FORMAT_A8B8G8R8,kRbgAtlasWidth,kRbgAtlasWidth,0)<0) {
            releaseRbgDecodedPages();g_rbgPageAllocationAttempted=true;
            return status(3,count,0,0);
        }
        for(auto* texture:{&g_rbgDecodedAtlasTexture}) {
            sceGxmTextureSetMinFilter(texture,SCE_GXM_TEXTURE_FILTER_POINT);
            sceGxmTextureSetMagFilter(texture,SCE_GXM_TEXTURE_FILTER_POINT);
        }
        g_rbgPageScratch=static_cast<std::uint32_t*>(std::malloc(lagi::RbgDecodedPage::Pixels*4));
        if(!g_rbgPageScratch) {
            releaseRbgDecodedPages();g_rbgPageAllocationAttempted=true;
            return status(3,count,0,0);
        }
    }
    if(!g_rbgAtlasPixels)return status(3,count,0,0);
    // Reserve every surviving required slot before evicting any old page.
    std::array<unsigned,kRbgPageSlots> slots{};slots.fill(kRbgPageSlots);
    std::array<bool,kRbgPageSlots> reserved{};
    for(unsigned i=0;i<count;++i)for(unsigned slot=0;slot<kRbgPageSlots;++slot)
        if(g_rbgPageAssigned[slot] && g_rbgPages[slot].key().base==required[i]) {
            slots[i]=slot;reserved[slot]=true;break;
        }
    for(unsigned i=0;i<count;++i)if(slots[i]==kRbgPageSlots)
        for(unsigned slot=0;slot<kRbgPageSlots;++slot)if(!reserved[slot]) {
            slots[i]=slot;reserved[slot]=true;g_rbgPageAssigned[slot]=true;break;
        }
    lagi::RbgDecodedPage::Generations generations{};
    static_assert(generations.size()==kSceneVdp2BlockCount);
    std::copy_n(sourceGenerations,generations.size(),generations.begin());
    unsigned rebuilt=0,hits=0;
    g_rbgPageLookup.fill(0);
    for(unsigned i=0;i<count;++i) {
        const unsigned slot=slots[i];
        const lagi::RbgPageKey key{required[i],static_cast<std::uint16_t>(rbg.format[1]),
            0,(static_cast<unsigned>(rbg.parameterFormat[0])&16)!=0,true}; // CAOS remains live on GPU.
        const auto decodeStart=sceKernelGetProcessTimeWide();
        const auto result=g_rbgPages[slot].update(raw,generations,key,
            g_rbgPageScratch,lagi::RbgDecodedPage::Pixels);
        decodeUs+=static_cast<unsigned>(sceKernelGetProcessTimeWide()-decodeStart);
        if(result==lagi::RbgPageResult::Unsupported)return status(4,count,rebuilt,hits);
        if(result==lagi::RbgPageResult::Rebuilt) {
            ++rebuilt;
            const auto copyStart=sceKernelGetProcessTimeWide();
            auto* atlas=static_cast<std::uint32_t*>(g_rbgAtlasPixels);
            const unsigned origin=(slot/2)*512*kRbgAtlasWidth+(slot%2)*512;
            for(unsigned y=0;y<512;++y)std::memcpy(atlas+origin+y*kRbgAtlasWidth,
                g_rbgPageScratch+y*512,512*4);
            atlasCopyUs+=static_cast<unsigned>(sceKernelGetProcessTimeWide()-copyStart);
        } else ++hits;
        g_rbgPageLookup[required[i]/2048]=slot;
    }
    auto encode=[&](const float* planes,float* output,unsigned format) {
        const unsigned width=(format&3)>=1?2:1,height=(format&3)>=2?2:1;
        for(unsigned plane=0;plane<16;++plane) {
            unsigned code=0;
            for(unsigned page=0;page<width*height;++page)
                code|=g_rbgPageLookup[static_cast<unsigned>(planes[plane])/2048+page]<<(page*2);
            output[plane]=static_cast<float>(code);
        }
    };
    if(mode!=1)encode(rbg.planesA,g_rbgDecodedPlanesA,static_cast<unsigned>(rbg.parameterFormat[0]));
    if(mode!=0)encode(rbg.planesB,g_rbgDecodedPlanesB,static_cast<unsigned>(rbg.parameterFormat[1]));
    return status(0,count,rebuilt,hits);
}
static bool prepareRbgDecodedPages() {
    return prepareRbgDecodedPagesFor(g_sceneRbg0,g_sceneVdp2Raw,g_sceneVdp2BlockGenerations,false);
}

// Included inside the Vita renderer namespace after mapped allocation
// helper declarations. Diagnostic reservation only: no GXM draw references.
#if NEPTUNE_GEOMETRY_ARENA_PROBE
static constexpr unsigned kGeometryArenaBytes =
    NEPTUNE_GEOMETRY_ARENA_MIB * 1024u * 1024u;
static SceUID g_geometryArenaUid = -1;
static void* g_geometryArenaMemory = nullptr;
static bool g_geometryArenaAttempted = false;
static bool g_geometryArenaCdram = false;
static std::optional<neptune::GeometryArena<4096>> g_geometryArena;

static void ensureGeometryArena()
{
    if (g_geometryArenaAttempted || !g_gxmInitialized) return;
    g_geometryArenaAttempted = true;
    const auto start = sceKernelGetProcessTimeWide();
    g_geometryArenaMemory = probeCdramAlloc(kGeometryArenaBytes,
        SCE_GXM_MEMORY_ATTRIB_READ, &g_geometryArenaUid);
    g_geometryArenaCdram = g_geometryArenaMemory != nullptr;
    if (!g_geometryArenaMemory)
        g_geometryArenaMemory = probeGpuAlloc(kGeometryArenaBytes,
            SCE_GXM_MEMORY_ATTRIB_READ, &g_geometryArenaUid);
    if (g_geometryArenaMemory)
        g_geometryArena.emplace(kGeometryArenaBytes);
    logging::writef("[NeptuneGeometryArena] requestedBytes=%u mappedBytes=%u "
        "ready=%u cdram=%u allocUs=%llu diagnostic=1\n",
        kGeometryArenaBytes, g_geometryArenaMemory ? kGeometryArenaBytes : 0u,
        unsigned(g_geometryArenaMemory != nullptr), unsigned(g_geometryArenaCdram),
        static_cast<unsigned long long>(sceKernelGetProcessTimeWide()-start));
}

#if NEPTUNE_GEOMETRY_OBSERVE
// Shadow residency: only the range metadata is exercised. No payload is
// copied into mapped storage and no GXM work references these allocations.
// This estimates a cumulative sampled working set, not actual cache hits.
using GeometryEstimate = std::pair<neptune::GeometryRepresentationKey,std::uint64_t>;
using GeometryShadowLru = std::list<neptune::GeometryRepresentationKey>;
static GeometryShadowLru g_geometryShadowLru;
struct GeometryShadowEntry {
    neptune::GeometryArena<4096>::Range range;
    std::uint64_t lastSample;
    GeometryShadowLru::iterator lru;
};
static std::unordered_map<neptune::GeometryRepresentationKey,GeometryShadowEntry,
    neptune::GeometryRepresentationKeyHash> g_geometryShadow;
static std::uint64_t g_geometryShadowSample = 0;

static void observeGeometryArena(const std::vector<GeometryEstimate>& estimates)
{
    if (!g_geometryArena) return;
    const auto start = sceKernelGetProcessTimeWide();
    auto& arena = *g_geometryArena;
    const auto sample = ++g_geometryShadowSample;
    unsigned hits=0, misses=0, admitted=0, evicted=0, skipped=0;
    // Pin all compatible existing ranges for this sample before eviction.
    // Size mismatch is not reuse; discard the provisional old estimate.
    for (const auto& item:estimates) {
        const auto it=g_geometryShadow.find(item.first);
        if (it==g_geometryShadow.end()) continue;
        if (it->second.range.bytes!=item.second) {
            arena.retire(it->second.range);
            g_geometryShadowLru.erase(it->second.lru);
            g_geometryShadow.erase(it);
        } else {
            it->second.lastSample=sample;
            g_geometryShadowLru.splice(g_geometryShadowLru.end(),g_geometryShadowLru,it->second.lru);
            ++hits;
        }
    }
    for (const auto& item:estimates) {
        if (g_geometryShadow.find(item.first)!=g_geometryShadow.end()) continue;
        ++misses;
        if (!item.second || item.second>arena.capacity()) { ++skipped; continue; }
        auto range=arena.allocate(static_cast<std::size_t>(item.second),64u);
        // The first entry is the oldest. All current-sample hits were
        // moved to the back before admissions; never evict those.
        constexpr unsigned maxEvictionsPerSample=16;
        while (!range && evicted<maxEvictionsPerSample &&
               !g_geometryShadowLru.empty()) {
            auto victim=g_geometryShadow.find(g_geometryShadowLru.front());
            if (victim==g_geometryShadow.end() ||
                victim->second.lastSample==sample) break;
            arena.retire(victim->second.range);
            g_geometryShadowLru.pop_front();
            g_geometryShadow.erase(victim);
            ++evicted;
            range=arena.allocate(static_cast<std::size_t>(item.second),64u);
        }
        if (!range) { ++skipped; continue; }
        g_geometryShadowLru.push_back(item.first);
        g_geometryShadow.emplace(item.first,GeometryShadowEntry{range,sample,std::prev(g_geometryShadowLru.end())});
        ++admitted;
    }
    logging::writef("[NeptuneGeometryArenaSample] sample=%llu frame=%llu "
        "mode=%u hits=%u misses=%u admitted=%u evicted=%u skipped=%u "
        "ranges=%u estimatedBytes=%u peakEstimatedBytes=%u largestFree=%u "
        "capacity=%u sampleUs=%llu shadow=1\n",
        static_cast<unsigned long long>(sample),
        static_cast<unsigned long long>(azel_bridge::published_frame_number()),
        unsigned(g_sceneGameMode),hits,misses,admitted,evicted,skipped,
        unsigned(arena.count()),unsigned(arena.used()),unsigned(arena.peak()),
        unsigned(arena.largestFree()),unsigned(arena.capacity()),
        static_cast<unsigned long long>(sceKernelGetProcessTimeWide()-start));
}
#endif

#if NEPTUNE_GEOMETRY_PAYLOAD_PROBE
// Independent, CPU-only validation of canonical transformed vertex bytes.
// Never binds this memory to a draw or trusts it as a rendering input.
static std::optional<neptune::GeometryArena<4096>> g_geometryPayloadRanges;
static std::unordered_map<neptune::GeometryRepresentationKey,
    neptune::GeometryArena<4096>::Range,
    neptune::GeometryRepresentationKeyHash> g_geometryPayloads;
static unsigned g_geometryPayloadUploads=0, g_geometryPayloadHits=0,
    g_geometryPayloadMismatch=0, g_geometryPayloadSkipped=0;
static std::uint64_t g_geometryPayloadBytes=0, g_geometryPayloadUs=0;

static void clearGeometryPayloads()
{
    g_geometryPayloads.clear();
    g_geometryPayloadRanges.reset();
    // The backing memory has no GPU readers in this diagnostic.
}

static void verifyGeometryPayload(const neptune::GeometryRepresentationKey& key,
    const void* source, std::size_t bytes)
{
    if (!g_geometryArenaMemory || !key.valid() || !source || !bytes) return;
    const auto started=sceKernelGetProcessTimeWide();
    if (!g_geometryPayloadRanges) g_geometryPayloadRanges.emplace(kGeometryArenaBytes);
    auto& ranges=*g_geometryPayloadRanges;
    auto* base=static_cast<unsigned char*>(g_geometryArenaMemory);
    auto it=g_geometryPayloads.find(key);
    if (it!=g_geometryPayloads.end()) {
        const auto range=it->second;
        if (range.bytes!=bytes || std::memcmp(base+range.offset,source,bytes)!=0)
            ++g_geometryPayloadMismatch;
        else ++g_geometryPayloadHits;
    } else if (bytes<=256u*1024u && g_geometryPayloadBytes<=256u*1024u-bytes) {
        auto range=ranges.allocate(bytes,64u);
        if (range) {
            std::memcpy(base+range.offset,source,bytes);
            if (std::memcmp(base+range.offset,source,bytes)!=0)
                ++g_geometryPayloadMismatch;
            g_geometryPayloads.emplace(key,range);
            ++g_geometryPayloadUploads;
            g_geometryPayloadBytes+=bytes;
        } else ++g_geometryPayloadSkipped;
    } else ++g_geometryPayloadSkipped;
    g_geometryPayloadUs+=sceKernelGetProcessTimeWide()-started;
}

static void reportGeometryPayloads()
{
    if (!g_geometryPayloadRanges) return;
    logging::writef("[NeptuneGeometryPayload] frame=%llu uploads=%u hits=%u "
        "mismatch=%u skipped=%u bytes=%llu used=%u ranges=%u timeUs=%llu cpuOnly=1\n",
        static_cast<unsigned long long>(azel_bridge::published_frame_number()),
        g_geometryPayloadUploads,g_geometryPayloadHits,g_geometryPayloadMismatch,
        g_geometryPayloadSkipped,static_cast<unsigned long long>(g_geometryPayloadBytes),
        unsigned(g_geometryPayloadRanges->used()),unsigned(g_geometryPayloadRanges->count()),
        static_cast<unsigned long long>(g_geometryPayloadUs));
    g_geometryPayloadUploads=g_geometryPayloadHits=g_geometryPayloadMismatch=g_geometryPayloadSkipped=0;
    g_geometryPayloadBytes=g_geometryPayloadUs=0;
}
#endif

static void releaseGeometryArena()
{
    // This probe never binds the block to a GXM draw. Before adding actual
    // submissions, require successful GPU completion before unmapping.
#if NEPTUNE_GEOMETRY_OBSERVE
    g_geometryShadow.clear();
    g_geometryShadowLru.clear();
    g_geometryShadowSample=0;
#endif
#if NEPTUNE_GEOMETRY_PAYLOAD_PROBE
    clearGeometryPayloads();
#endif
    g_geometryArena.reset();
    if (g_geometryArenaUid >= 0) {
        if (g_geometryArenaMemory) sceGxmUnmapMemory(g_geometryArenaMemory);
        sceKernelFreeMemBlock(g_geometryArenaUid);
    }
    g_geometryArenaUid = -1;
    g_geometryArenaMemory = nullptr;
    g_geometryArenaAttempted = false;
    g_geometryArenaCdram = false;
}
#endif

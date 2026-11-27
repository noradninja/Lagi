// Included inside the Vita renderer namespace after mapped allocation
// helper declarations. Diagnostic reservation only: no GXM draw references.
#if NEPTUNE_GEOMETRY_ARENA_PROBE
static constexpr unsigned kGeometryArenaBytes =
    NEPTUNE_GEOMETRY_ARENA_MIB * 1024u * 1024u;
static SceUID g_geometryArenaUid = -1;
static void* g_geometryArenaMemory = nullptr;
static bool g_geometryArenaAttempted = false;
static bool g_geometryArenaCdram = false;
static std::optional<neptune::GeometryArena<>> g_geometryArena;

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
struct GeometryShadowEntry {
    neptune::GeometryArena<>::Range range;
    std::uint64_t lastSample;
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
            g_geometryShadow.erase(it);
        } else {
            it->second.lastSample=sample;
            ++hits;
        }
    }
    for (const auto& item:estimates) {
        if (g_geometryShadow.find(item.first)!=g_geometryShadow.end()) continue;
        ++misses;
        if (!item.second || item.second>arena.capacity()) { ++skipped; continue; }
        auto range=arena.allocate(static_cast<std::size_t>(item.second),64u);
        while (!range) {
            auto victim=g_geometryShadow.end();
            for (auto it=g_geometryShadow.begin();it!=g_geometryShadow.end();++it) {
                if (it->second.lastSample==sample) continue;
                if (victim==g_geometryShadow.end() ||
                    it->second.lastSample<victim->second.lastSample ||
                    (it->second.lastSample==victim->second.lastSample &&
                     it->second.range.id<victim->second.range.id)) victim=it;
            }
            if (victim==g_geometryShadow.end()) break;
            arena.retire(victim->second.range);
            g_geometryShadow.erase(victim);
            ++evicted;
            range=arena.allocate(static_cast<std::size_t>(item.second),64u);
        }
        if (!range) { ++skipped; continue; }
        g_geometryShadow.emplace(item.first,GeometryShadowEntry{range,sample});
        ++admitted;
    }
    logging::writef("[NeptuneGeometryArenaSample] sample=%llu frame=%llu "
        "mode=%u hits=%u misses=%u admitted=%u evicted=%u skipped=%u "
        "ranges=%u estimatedBytes=%u peakEstimatedBytes=%u largestFree=%u "
        "capacity=%u sampleUs=%llu shadow=1\\n",
        static_cast<unsigned long long>(sample),
        static_cast<unsigned long long>(azel_bridge::published_frame_number()),
        unsigned(g_sceneGameMode),hits,misses,admitted,evicted,skipped,
        unsigned(arena.count()),unsigned(arena.used()),unsigned(arena.peak()),
        unsigned(arena.largestFree()),unsigned(arena.capacity()),
        static_cast<unsigned long long>(sceKernelGetProcessTimeWide()-start));
}
#endif

static void releaseGeometryArena()
{
    // This probe never binds the block to a GXM draw. Before adding actual
    // submissions, require successful GPU completion before unmapping.
#if NEPTUNE_GEOMETRY_OBSERVE
    g_geometryShadow.clear();
    g_geometryShadowSample=0;
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

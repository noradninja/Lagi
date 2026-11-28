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

#if NEPTUNE_RESIDENT_SUBDIV_INDICES
// One physical allocation domain for all GPU-addressable arena ranges.
// Topology and split streams are renderer-lifetime reservations; future model
// ranges must allocate from this same domain and retire at a proven GPU boundary.
// Shadow estimates remain independent metadata and never write mapped bytes.
// CPU payload probe is mutually exclusive with resident indices.
static std::optional<neptune::GeometryArena<4096>> g_geometryPhysicalRanges;
#if NEPTUNE_RESIDENT_MODEL_GEOMETRY
static neptune::GeometrySubmissionEpoch g_geometrySubmissionEpoch;
static unsigned g_geometryEpochCompletions=0, g_geometryEpochFaults=0;

static void faultGeometryEpoch()
{
    if (!g_geometrySubmissionEpoch.failed()) {
        ++g_geometryEpochFaults;
        logging::writef("[NeptuneGeometryEpoch] disabled=1 ambiguousBoundary=1\n");
    }
    g_geometrySubmissionEpoch.fail();
}

// Created only after successful native scene begin. The scope spans world
// composition, including any intermediate end/begin and diagnostic finish.
// No new GPU wait is introduced. An unhandled exit fails closed.
class GeometryFrameEpochScope {
public:
    GeometryFrameEpochScope() {
        if (g_geometryPhysicalRanges && !g_geometrySubmissionEpoch.failed())
            serial_=g_geometrySubmissionEpoch.begin();
    }
    ~GeometryFrameEpochScope() {
        if (serial_) faultGeometryEpoch();
    }
    void finish(bool endSceneSucceeded) {
        if (!serial_) return;
        serial_=0;
        if (!endSceneSucceeded || g_geometrySubmissionEpoch.failed()) {
            faultGeometryEpoch();
            return;
        }
        const auto completed=g_geometrySubmissionEpoch.complete(true);
        if (!completed || !g_geometryPhysicalRanges->completeThrough(completed)) {
            faultGeometryEpoch();
            return;
        }
        ++g_geometryEpochCompletions;
    }
private:
    std::uint64_t serial_=0;
};
#endif
static const std::uint16_t* g_residentTopology = nullptr;
static bool g_residentTopologyAttempted = false;
static neptune::GeometryArena<4096>::Range g_residentTopologyRange{};
static unsigned g_residentTopologyDraws=0, g_residentTopologyFallbacks=0;
static std::uint64_t g_residentTopologyIndices=0;

static void ensureResidentSubdivisionTopology()
{
    if (g_residentTopologyAttempted || !g_geometryArenaMemory) return;
    g_residentTopologyAttempted=true;
    constexpr unsigned count=neptune::kSubdivisionMaxQuads*neptune::kSubdivisionIndicesPerQuad;
    constexpr unsigned bytes=count*sizeof(std::uint16_t);
    const auto started=sceKernelGetProcessTimeWide();
    g_geometryPhysicalRanges.emplace(kGeometryArenaBytes);
    auto range=g_geometryPhysicalRanges->allocate(bytes,64u);
    if(range) {
        auto* target=reinterpret_cast<std::uint16_t*>(
            static_cast<unsigned char*>(g_geometryArenaMemory)+range.offset);
        if(neptune::writeSubdivisionTopology(target,count,neptune::kSubdivisionMaxQuads)) {
            g_residentTopology=target;
            g_residentTopologyRange=range;
        } else g_geometryPhysicalRanges->retire(range);
    }
    logging::writef("[NeptuneResidentTopology] ready=%u cdram=%u bytes=%u "
        "quads=%u uploadUs=%llu immutable=1\n",
        unsigned(g_residentTopology!=nullptr),unsigned(g_geometryArenaCdram),
        g_residentTopology?bytes:0u,neptune::kSubdivisionMaxQuads,
        static_cast<unsigned long long>(sceKernelGetProcessTimeWide()-started));
}

#if NEPTUNE_SPLIT_SUBDIV_STREAMS
static neptune::SubdivisionGeometryVertex* g_splitGeometry=nullptr;
static neptune::SubdivisionShadeVertex* g_splitShade=nullptr;
static bool g_splitAttempted=false, g_splitMirrorsReady=false;
static constexpr unsigned kSplitVertexCount=neptune::kSubdivisionMaxQuads*9u;
static unsigned g_splitDrawFrames=0, g_splitFallbackFrames=0;
#if NEPTUNE_SPLIT_SUBDIV_PRIMARY
static std::uint64_t g_splitAvoidedBytes=0, g_splitFallbackBytes=0;
// Storage authority is selected when subdivision storage is prepared.
// Do not change it on view switches or failed per-draw bindings.
static bool splitSubdivisionPrimary() noexcept
{
    return g_splitMirrorsReady && g_splitSubdivVertexProgram;
}

static SubdivGouraudVertex readPrimarySubdivisionVertex(unsigned index)
{
    const auto& geom=g_splitGeometry[index];
    const auto& shade=g_splitShade[index];
    return neptune::interleaveSubdivisionVertex(geom,shade);
}
#endif

static void ensureSplitSubdivisionStreams()
{
    if (g_splitAttempted || !g_geometryPhysicalRanges || !g_residentTopology) return;
    g_splitAttempted=true;
    auto& arena=*g_geometryPhysicalRanges;
    constexpr auto geometryBytes=kSplitVertexCount*sizeof(neptune::SubdivisionGeometryVertex);
    constexpr auto shadeBytes=kSplitVertexCount*sizeof(neptune::SubdivisionShadeVertex);
    auto geometry=arena.allocate(geometryBytes,64u);
    auto shade=geometry ? arena.allocate(shadeBytes,64u) : neptune::GeometryArena<4096>::Range{};
    if(geometry && shade) {
        auto* base=static_cast<unsigned char*>(g_geometryArenaMemory);
        g_splitGeometry=reinterpret_cast<neptune::SubdivisionGeometryVertex*>(base+geometry.offset);
        g_splitShade=reinterpret_cast<neptune::SubdivisionShadeVertex*>(base+shade.offset);
    } else if(geometry) arena.retire(geometry);
    logging::writef("[NeptuneSplitStreams] ready=%u geometryBytes=%u shadeBytes=%u capacity=%u cdram=%u\n",
        unsigned(g_splitGeometry!=nullptr),g_splitGeometry?unsigned(geometryBytes):0u,
        g_splitShade?unsigned(shadeBytes):0u,kSplitVertexCount,unsigned(g_geometryArenaCdram));
}

static void reportSplitSubdivisionStreams()
{
    if(!g_splitDrawFrames && !g_splitFallbackFrames) return;
    logging::writef("[NeptuneSplitStreamsWindow] frame=%llu mode=%u split=%u fallback=%u\n",
        static_cast<unsigned long long>(azel_bridge::published_frame_number()),
        unsigned(g_sceneGameMode),g_splitDrawFrames,g_splitFallbackFrames);
#if NEPTUNE_SPLIT_SUBDIV_PRIMARY
    logging::writef("[NeptuneSplitPrimaryWindow] frame=%llu mode=%u avoidedBytes=%llu fallbackBytes=%llu\n",
        static_cast<unsigned long long>(azel_bridge::published_frame_number()),
        unsigned(g_sceneGameMode),static_cast<unsigned long long>(g_splitAvoidedBytes),
        static_cast<unsigned long long>(g_splitFallbackBytes));
    g_splitAvoidedBytes=g_splitFallbackBytes=0;
#endif
    g_splitDrawFrames=g_splitFallbackFrames=0;
}
#endif

static void reportResidentSubdivisionTopology()
{
    if(!g_residentTopologyDraws && !g_residentTopologyFallbacks) return;
    logging::writef("[NeptuneResidentTopologyFrame] frame=%llu mode=%u draws=%u "
        "fallbacks=%u indices=%llu\n",
        static_cast<unsigned long long>(azel_bridge::published_frame_number()),
        unsigned(g_sceneGameMode),g_residentTopologyDraws,g_residentTopologyFallbacks,
        static_cast<unsigned long long>(g_residentTopologyIndices));
    g_residentTopologyDraws=g_residentTopologyFallbacks=0;
    g_residentTopologyIndices=0;
}
#endif

static void releaseGeometryArena()
{
    // The caller has stopped the render thread and destroyed the GXM context
    // after its final finish. The immutable table was never modified or freed
    // while any context could reference it.
#if NEPTUNE_RESIDENT_SUBDIV_INDICES
    reportResidentSubdivisionTopology();
    g_residentTopology=nullptr;
#if NEPTUNE_SPLIT_SUBDIV_STREAMS
    reportSplitSubdivisionStreams();
    g_splitGeometry=nullptr;g_splitShade=nullptr;
    g_splitAttempted=g_splitMirrorsReady=false;
#endif
#if NEPTUNE_RESIDENT_MODEL_GEOMETRY
    logging::writef("[NeptuneGeometryEpoch] completions=%u faults=%u shutdown=1\n",
        g_geometryEpochCompletions,g_geometryEpochFaults);
    g_geometrySubmissionEpoch={};
    g_geometryEpochCompletions=g_geometryEpochFaults=0;
#endif
    g_geometryPhysicalRanges.reset();
    g_residentTopologyRange={};
    g_residentTopologyAttempted=false;
#endif
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

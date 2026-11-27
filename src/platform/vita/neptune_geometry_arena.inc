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

static void releaseGeometryArena()
{
    // This probe never binds the block to a GXM draw. Before adding actual
    // submissions, require successful GPU completion before unmapping.
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

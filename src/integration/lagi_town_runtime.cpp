#include "lagi/lagi_town_runtime.h"

#include "lagi/lagi_town_bootstrap.h"
#include "lagi/disc_image.h"
#include "lagi/platform.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <map>
#include <memory>

s8 readSaturnS8(sSaturnPtr ptr);
s32 readSaturnS32(sSaturnPtr ptr);
u32 readSaturnU32(sSaturnPtr ptr);
fixedPoint readSaturnFP(sSaturnPtr ptr);
sSaturnPtr readSaturnEA(sSaturnPtr ptr);

namespace lagi::azel {

namespace {

using NativeVec3S16 = std::array<s16, 3>;
using NativeVec3U16 = std::array<u16, 3>;

struct NativeModelQuadExtra {
    NativeVec3S16 normals{};
    NativeVec3U16 colors{};
};

struct NativeModelQuad {
    std::array<u16, 4> indices{};
    u16 lightingControl = 0;
    u16 cmdCtrl = 0;
    u16 cmdPmod = 0;
    u16 cmdColr = 0;
    u16 cmdSrca = 0;
    u16 cmdSize = 0;
    std::vector<NativeModelQuadExtra> extraData;
};

// CPU prefix of Azel's sProcessed3dModel. The Vita build deliberately omits
// the BGFX-only tail, while retaining the exact fields consumed at the
// addObjectToDrawList boundary.
struct NativeProcessedModel {
    u8* base = nullptr;
    fixedPoint radius{};
    u32 numVertices = 0;
    std::vector<NativeVec3S16> vertices;
    std::vector<NativeModelQuad> quads;
};

} // namespace

struct TownRuntimeBundle::ModelCache {
    std::map<std::uint32_t, std::unique_ptr<NativeProcessedModel>> models;
};

static TownRuntimeState g_runtime{};

static std::uint16_t be16(const std::uint8_t* p)
{
    return static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(p[0]) << 8) |
        static_cast<std::uint16_t>(p[1]));
}

static std::uint32_t be32(const std::uint8_t* p)
{
    return
        (static_cast<std::uint32_t>(p[0]) << 24) |
        (static_cast<std::uint32_t>(p[1]) << 16) |
        (static_cast<std::uint32_t>(p[2]) << 8) |
        static_cast<std::uint32_t>(p[3]);
}

static std::int16_t bes16(const std::uint8_t* p)
{
    return static_cast<std::int16_t>(be16(p));
}

static std::int32_t bes32(const std::uint8_t* p)
{
    return static_cast<std::int32_t>(be32(p));
}

static bool parse_collision_model(
    const std::vector<std::uint8_t>& bundle,
    std::uint32_t tableOffset,
    TownRuntimeCollisionModel& out)
{
    out = {};
    if (tableOffset + 4u > bundle.size())
        return false;

    const std::uint32_t modelOffset =
        be32(bundle.data() + tableOffset);
    if (!modelOffset || modelOffset + 12u > bundle.size())
        return false;

    const std::uint8_t* raw = bundle.data() + modelOffset;
    out.radius =
        static_cast<float>(bes32(raw + 0)) / 65536.0f;
    const std::uint32_t vertexCount = be32(raw + 4);
    const std::uint32_t verticesOffset = be32(raw + 8);

    if (!vertexCount || vertexCount > 255u ||
        static_cast<std::uint64_t>(verticesOffset) +
            static_cast<std::uint64_t>(vertexCount) * 6u >
            bundle.size())
        return false;

    out.vertices.reserve(vertexCount);
    for (std::uint32_t i = 0; i < vertexCount; ++i) {
        const std::uint8_t* v =
            bundle.data() + verticesOffset + i * 6u;
        out.vertices.push_back({
            static_cast<float>(bes16(v + 0)) / 4096.0f,
            static_cast<float>(bes16(v + 2)) / 4096.0f,
            static_cast<float>(bes16(v + 4)) / 4096.0f
        });
    }

    std::uint32_t p = modelOffset + 0x0Cu;
    constexpr unsigned kMaxCollisionQuads = 4096;
    for (unsigned q = 0; q < kMaxCollisionQuads; ++q) {
        if (p + 8u > bundle.size())
            return false;

        TownRuntimeCollisionQuad quad{};
        for (unsigned i = 0; i < 4; ++i)
            quad.indices[i] =
                be16(bundle.data() + p + i * 2u);

        if (!quad.indices[0] && !quad.indices[1] &&
            !quad.indices[2] && !quad.indices[3])
            return !out.quads.empty();

        for (unsigned i = 0; i < 4; ++i) {
            if (quad.indices[i] >= vertexCount)
                return false;
        }

        p += 8u;
        if (p + 12u > bundle.size())
            return false;

        const std::uint16_t lightingControl =
            be16(bundle.data() + p + 0);
        quad.cmdSrca = be16(bundle.data() + p + 8);
        quad.onCollisionScriptIndex =
            be16(bundle.data() + p + 10);
        p += 12u;

        const unsigned lightingMode =
            (lightingControl >> 8) & 3u;
        if (lightingMode == 1u) {
            if (p + 8u > bundle.size())
                return false;
            quad.normal[0] =
                static_cast<float>(bes16(bundle.data() + p + 0)) /
                4096.0f;
            quad.normal[1] =
                static_cast<float>(bes16(bundle.data() + p + 2)) /
                4096.0f;
            quad.normal[2] =
                static_cast<float>(bes16(bundle.data() + p + 4)) /
                4096.0f;
            p += 8u;
        } else if (lightingMode == 2u) {
            if (p + 48u > bundle.size())
                return false;
            quad.normal[0] =
                static_cast<float>(bes16(bundle.data() + p + 0)) /
                4096.0f;
            quad.normal[1] =
                static_cast<float>(bes16(bundle.data() + p + 2)) /
                4096.0f;
            quad.normal[2] =
                static_cast<float>(bes16(bundle.data() + p + 4)) /
                4096.0f;
            p += 48u;
        } else if (lightingMode == 3u) {
            if (p + 24u > bundle.size())
                return false;
            quad.normal[0] =
                static_cast<float>(bes16(bundle.data() + p + 0)) /
                4096.0f;
            quad.normal[1] =
                static_cast<float>(bes16(bundle.data() + p + 2)) /
                4096.0f;
            quad.normal[2] =
                static_cast<float>(bes16(bundle.data() + p + 4)) /
                4096.0f;
            p += 24u;
        }

        out.quads.push_back(quad);
    }

    return false;
}

static bool build_runtime_collision_data()
{
    const std::vector<std::uint8_t>* bundle =
        town_runtime_resource(
            g_runtime.setupNpcFileIndex == 0
                ? "COMMON3.MCB"
                : "RUINMP.MCB");
    sSaturnMemoryFile* overlay = town_overlay_file();
    if (!bundle || !overlay)
        return false;

    unsigned instances = 0;
    unsigned models = 0;
    unsigned quads = 0;

    for (auto& cell : g_runtime.cells) {
        cell.collisionInstances.clear();
        if (!cell.valid || !cell.collisionListEA)
            continue;

        sSaturnPtr entry =
            overlay->getSaturnPtr(cell.collisionListEA);

        constexpr unsigned kMaxInstances = 1024;
        for (unsigned i = 0; i < kMaxInstances; ++i, entry += 0x10) {
            const std::uint32_t modelTableOffset =
                readSaturnU32(entry + 0);
            if (!modelTableOffset)
                break;

            TownRuntimeCollisionInstance instance{};
            instance.modelTableOffset = modelTableOffset;
            instance.position[0] =
                static_cast<float>(readSaturnS32(entry + 4)) /
                65536.0f;
            instance.position[1] =
                static_cast<float>(readSaturnS32(entry + 8)) /
                65536.0f;
            instance.position[2] =
                static_cast<float>(readSaturnS32(entry + 12)) /
                65536.0f;

            if (!parse_collision_model(
                    *bundle,
                    modelTableOffset,
                    instance.model)) {
                lagi::platform::logging::writef(
                    "[TownRuntime] collision model decode failed table=0x%X\n",
                    modelTableOffset);
                return false;
            }

            quads +=
                static_cast<unsigned>(instance.model.quads.size());
            ++models;
            ++instances;
            cell.collisionInstances.push_back(std::move(instance));
        }
    }

    lagi::platform::logging::writef(
        "[TownRuntime] collision data: instances=%u models=%u quads=%u source=%s\n",
        instances, models, quads,
        g_runtime.setupNpcFileIndex == 0
            ? "COMMON3.MCB"
            : "RUINMP.MCB");

    char status[78];
    std::snprintf(
        status, sizeof(status),
        "[PASS] RUIN COLLISION %u MODELS / %u QUADS",
        models, quads);
    lagi::platform::renderer::status(status, 0xFF70E0A0u);

    return instances != 0u;
}

const TownRuntimeState& town_runtime()
{
    return g_runtime;
}

const TownRuntimeCell* town_runtime_active_cell()
{
    if (!g_runtime.initialized ||
        g_runtime.activeCellIndex < 0 ||
        g_runtime.activeCellIndex >=
            static_cast<int>(g_runtime.cells.size()))
        return nullptr;

    const TownRuntimeCell& cell =
        g_runtime.cells[static_cast<std::size_t>(g_runtime.activeCellIndex)];
    return cell.valid ? &cell : nullptr;
}

const std::vector<std::uint8_t>* town_runtime_resource(const char* name)
{
    if (!name)
        return nullptr;

    for (const auto& resource : g_runtime.resources) {
        if (resource.name == name)
            return &resource.bytes;
    }
    return nullptr;
}

static std::unique_ptr<NativeProcessedModel> parse_native_model(
    const TownRuntimeResource& resource,
    std::uint32_t tableOffset)
{
    const auto& bytes = resource.bytes;
    if (tableOffset + 4u > bytes.size()) return nullptr;
    const std::uint32_t modelOffset = be32(bytes.data() + tableOffset);
    if (!modelOffset || modelOffset + 12u > bytes.size()) return nullptr;

    auto model = std::make_unique<NativeProcessedModel>();
    model->base = const_cast<u8*>(bytes.data());
    model->radius = bes32(bytes.data() + modelOffset);
    model->numVertices = be32(bytes.data() + modelOffset + 4u);
    const std::uint32_t verticesOffset =
        be32(bytes.data() + modelOffset + 8u);
    if (!model->numVertices || model->numVertices > 65535u ||
        static_cast<std::uint64_t>(verticesOffset) +
            static_cast<std::uint64_t>(model->numVertices) * 6u >
            bytes.size())
        return nullptr;

    model->vertices.reserve(model->numVertices);
    for (std::uint32_t i = 0; i < model->numVertices; ++i) {
        const auto* v = bytes.data() + verticesOffset + i * 6u;
        model->vertices.push_back({bes16(v), bes16(v + 2), bes16(v + 4)});
    }

    std::uint32_t cursor = modelOffset + 12u;
    constexpr unsigned kMaxQuads = 16384;
    for (unsigned q = 0; q < kMaxQuads; ++q) {
        if (cursor + 8u > bytes.size()) return nullptr;
        NativeModelQuad quad{};
        for (unsigned i = 0; i < 4u; ++i)
            quad.indices[i] = be16(bytes.data() + cursor + i * 2u);
        if (!quad.indices[0] && !quad.indices[1] &&
            !quad.indices[2] && !quad.indices[3])
            return model->quads.empty() ? nullptr : std::move(model);
        for (const auto index : quad.indices)
            if (index >= model->numVertices) return nullptr;
        cursor += 8u;
        if (cursor + 12u > bytes.size()) return nullptr;
        quad.lightingControl = be16(bytes.data() + cursor + 0u);
        quad.cmdCtrl = be16(bytes.data() + cursor + 2u);
        quad.cmdPmod = be16(bytes.data() + cursor + 4u);
        quad.cmdColr = be16(bytes.data() + cursor + 6u);
        quad.cmdSrca = be16(bytes.data() + cursor + 8u);
        quad.cmdSize = be16(bytes.data() + cursor + 10u);
        cursor += 12u;

        const unsigned mode = (quad.lightingControl >> 8) & 3u;
        const unsigned count = mode == 1u ? 1u :
            ((mode == 2u || mode == 3u) ? 4u : 0u);
        const bool colors = mode == 2u;
        for (unsigned i = 0; i < count; ++i) {
            const unsigned bytesNeeded = colors ? 12u : 6u;
            if (cursor + bytesNeeded > bytes.size()) return nullptr;
            NativeModelQuadExtra extra{};
            extra.normals = {
                bes16(bytes.data() + cursor + 0u),
                bes16(bytes.data() + cursor + 2u),
                bes16(bytes.data() + cursor + 4u)};
            cursor += 6u;
            if (colors) {
                extra.colors = {
                    be16(bytes.data() + cursor + 0u),
                    be16(bytes.data() + cursor + 2u),
                    be16(bytes.data() + cursor + 4u)};
                cursor += 6u;
            }
            quad.extraData.push_back(extra);
        }
        if (mode == 1u) {
            if (cursor + 2u > bytes.size()) return nullptr;
            cursor += 2u;
        }
        model->quads.push_back(std::move(quad));
    }
    return nullptr;
}

const TownRuntimeBundle* town_runtime_bundle(std::int8_t fileIndex)
{
    for (const auto& bundle : g_runtime.bundles)
        if (bundle.fileIndex == fileIndex)
            return &bundle;
    return nullptr;
}

sProcessed3dModel* town_runtime_model(
    std::int8_t fileIndex,
    std::uint32_t tableOffset)
{
    if (!tableOffset) return nullptr;
    for (auto& bundle : g_runtime.bundles) {
        if (bundle.fileIndex != fileIndex || !bundle.model)
            continue;
        if (!bundle.modelCache)
            bundle.modelCache =
                std::make_shared<TownRuntimeBundle::ModelCache>();
        auto& models = bundle.modelCache->models;
        const auto found = models.find(tableOffset);
        if (found != models.end())
            return reinterpret_cast<sProcessed3dModel*>(found->second.get());
        auto model = parse_native_model(*bundle.model, tableOffset);
        if (!model) return nullptr;
        NativeProcessedModel* const result = model.get();
        models.emplace(tableOffset, std::move(model));
        return reinterpret_cast<sProcessed3dModel*>(result);
    }
    return nullptr;
}

bool acquire_town_runtime_bundle(std::int8_t fileIndex)
{
    for (auto& bundle : g_runtime.bundles) {
        if (bundle.fileIndex != fileIndex)
            continue;
        if (!bundle.model || !bundle.graphics)
            return false;
        ++bundle.refCount;
        return true;
    }
    return false;
}

void release_town_runtime_bundle(std::int8_t fileIndex)
{
    for (auto& bundle : g_runtime.bundles) {
        if (bundle.fileIndex == fileIndex && bundle.refCount) {
            --bundle.refCount;
            if (!bundle.refCount)
                bundle.modelCache.reset();
            return;
        }
    }
}

static bool load_runtime_resource(const char* name)
{
    TownRuntimeResource resource{};
    resource.name = name;
    if (!lagi::disc::read_file(name, resource.bytes) ||
        resource.bytes.empty()) {
        lagi::platform::logging::writef(
            "[TownRuntime] failed resource load: %s\n", name);
        return false;
    }

    lagi::platform::logging::writef(
        "[TownRuntime] resource %-12s %u bytes\n",
        name,
        static_cast<unsigned>(resource.bytes.size()));
    g_runtime.resources.push_back(std::move(resource));
    return true;
}

void update_town_runtime_active_cell(float worldX, float worldZ)
{
    if (!g_runtime.initialized || g_runtime.cells.empty()) {
        g_runtime.activeCellIndex = -1;
        return;
    }

    // The first Ruins scene is a 1x1 grid. For future multi-cell towns this
    // already centralizes selection in the scene runtime. Until the exact
    // Azel ring-buffer grid task is ported, choose the closest valid cell
    // origin in X/Z rather than letting rendering code own cell selection.
    int best = -1;
    float bestDist2 = 0.0f;
    for (std::size_t i = 0; i < g_runtime.cells.size(); ++i) {
        const auto& cell = g_runtime.cells[i];
        if (!cell.valid)
            continue;

        const float dx = worldX - cell.origin[0];
        const float dz = worldZ - cell.origin[2];
        const float d2 = dx*dx + dz*dz;
        if (best < 0 || d2 < bestDist2) {
            best = static_cast<int>(i);
            bestDist2 = d2;
        }
    }
    g_runtime.activeCellIndex = best;
}

bool init_town_runtime()
{
    g_runtime = {};

    const TownBootstrapInfo& boot = town_bootstrap_info();
    sSaturnMemoryFile* overlay = town_overlay_file();
    if (!boot.valid || !overlay || !overlay->m_data) {
        lagi::platform::logging::writef(
            "[TownRuntime] bootstrap/overlay unavailable\n");
        return false;
    }

    g_runtime.overlayFile = boot.overlayFile;
    g_runtime.setupNpcFileIndex = boot.setupNpcFileIndex;
    g_runtime.gridWidth = boot.gridWidth;
    g_runtime.gridHeight = boot.gridHeight;
    g_runtime.gridCellSize =
        static_cast<float>(boot.gridCellSize) / 65536.0f;
    g_runtime.initialScriptEA = 0x06054398u;
    g_runtime.scriptTableEA = boot.scriptTableEA;
    g_runtime.edgeEA = boot.edgeEA;
    g_runtime.envLcsTargetCount = boot.envLcsTargetCount;
    g_runtime.envLcsTargetEA = boot.envLcsTargetEA;

    // readTownSetup(..., 12) in TWN_RUIN_data.
    constexpr unsigned kRuinScriptCount = 12;
    const sSaturnPtr scriptTable =
        overlay->getSaturnPtr(boot.scriptTableEA);
    g_runtime.townScripts.reserve(kRuinScriptCount);
    for (unsigned i = 0; i < kRuinScriptCount; ++i) {
        const sSaturnPtr script =
            readSaturnEA(scriptTable + i * 4u);
        g_runtime.townScripts.push_back(
            static_cast<std::uint32_t>(script.m_offset));
    }

    const unsigned cellCount =
        static_cast<unsigned>(boot.gridWidth) *
        static_cast<unsigned>(boot.gridHeight);
    g_runtime.cells.resize(cellCount);

    const sSaturnPtr grid =
        overlay->getSaturnPtr(boot.gridEA);
    unsigned validCells = 0;
    for (unsigned i = 0; i < cellCount; ++i) {
        const sSaturnPtr cell =
            readSaturnEA(grid + i * 4u);

        TownRuntimeCell& dst = g_runtime.cells[i];
        if (cell.isNull())
            continue;

        dst.ea = static_cast<std::uint32_t>(cell.m_offset);
        dst.origin[0] =
            static_cast<float>(readSaturnS32(cell + 0x00)) / 65536.0f;
        dst.origin[1] =
            static_cast<float>(readSaturnS32(cell + 0x04)) / 65536.0f;
        dst.origin[2] =
            static_cast<float>(readSaturnS32(cell + 0x08)) / 65536.0f;
        dst.staticObjectListEA =
            static_cast<std::uint32_t>(
                readSaturnEA(cell + 0x0C).m_offset);
        dst.billboardListEA =
            static_cast<std::uint32_t>(
                readSaturnEA(cell + 0x10).m_offset);
        dst.collisionListEA =
            static_cast<std::uint32_t>(
                readSaturnEA(cell + 0x14).m_offset);
        dst.valid = true;
        ++validCells;
    }

    // Match overlayStart_TWN_RUIN's scene-owned file set. Keeping the bytes
    // here is the first step away from each debug adapter re-reading the disc.
    static constexpr std::array<const char*, 7> kResources = {{
        "COMMON3.MCB",
        "COMMON3.CGB",
        "RUINMP.MCB",
        "RUINMP.CGB",
        "RUINSCR.SCB",
        "RUINSCR.PNB",
        "EVTRUIN.FNT",
    }};

    g_runtime.resources.reserve(kResources.size());
    for (const char* name : kResources) {
        if (!load_runtime_resource(name))
            return false;
    }

    const auto resourceByName = [](const char* name) -> const TownRuntimeResource* {
        for (const auto& resource : g_runtime.resources)
            if (resource.name == name)
                return &resource;
        return nullptr;
    };
    g_runtime.bundles = {
        {0, resourceByName("COMMON3.MCB"), resourceByName("COMMON3.CGB")},
        {2, resourceByName("RUINMP.MCB"), resourceByName("RUINMP.CGB")},
    };

    // Mirror the allocation order used by upstream createEnvironmentTask2Sub0Sub0().
    // Azel patches every processed model's CMDCOLR/CMDSRCA with these VDP1
    // address-unit bases. Retain the allocation metadata at the platform
    // boundary so the Vita texture adapter can select the owning CGB and
    // recover its bundle-relative descriptor without changing Azel.
    constexpr std::uint32_t kTownVdp1Start = 0x25C18800u >> 3;
    constexpr std::uint32_t kTownVdp1Units = 0x63800u >> 3;
    std::uint32_t nextVdp1Unit = kTownVdp1Start + kTownVdp1Units;
    for (auto& bundle : g_runtime.bundles) {
        if (!bundle.graphics)
            continue;
        const std::uint32_t alignedBytes =
            (static_cast<std::uint32_t>(bundle.graphics->bytes.size()) +
             0x1Fu) & ~0x1Fu;
        const std::uint32_t allocationUnits = alignedBytes >> 3;
        if (allocationUnits > nextVdp1Unit - kTownVdp1Start)
            return false;
        nextVdp1Unit -= allocationUnits;
        bundle.vdp1Base = static_cast<std::uint16_t>(nextVdp1Unit);
        bundle.vdp1SizeUnits = static_cast<std::uint16_t>(allocationUnits);
        lagi::platform::logging::writef(
            "[TownRuntime] VDP1 bundle %d base=%04X units=%04X graphics=%s\n",
            static_cast<int>(bundle.fileIndex),
            static_cast<unsigned>(bundle.vdp1Base),
            static_cast<unsigned>(bundle.vdp1SizeUnits),
            bundle.graphics->name.c_str());
    }

    g_runtime.initialized = validCells != 0;
    if (!g_runtime.initialized)
        return false;

    if (!build_runtime_collision_data())
        return false;

    update_town_runtime_active_cell(
        g_runtime.cells[0].origin[0],
        g_runtime.cells[0].origin[2]);

    lagi::platform::logging::writef(
        "[TownRuntime] %s scene owner ready: grid=%dx%d cells=%u/%u scripts=%u resources=%u active=%d\n",
        g_runtime.overlayFile.c_str(),
        static_cast<int>(g_runtime.gridWidth),
        static_cast<int>(g_runtime.gridHeight),
        validCells,
        cellCount,
        static_cast<unsigned>(g_runtime.townScripts.size()),
        static_cast<unsigned>(g_runtime.resources.size()),
        g_runtime.activeCellIndex);

    lagi::platform::renderer::status(
        "[PASS] TWN_RUIN NATIVE SCENE OWNER",
        0xFF70E0A0u);
    return true;
}

} // namespace lagi::azel

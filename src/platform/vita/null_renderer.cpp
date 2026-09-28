#include "lagi/platform.h"
#include "lagi/debug_mesh.h"

#include <psp2/display.h>
#include <psp2/gxm.h>
#include <psp2/kernel/sysmem.h>

#include <algorithm>
#include <cmath>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace lagi::platform::renderer {

static constexpr int kWidth = 960;
static constexpr int kHeight = 544;
static constexpr int kPitch = 1024;
static constexpr std::size_t kFrameBytes =
    static_cast<std::size_t>(kPitch) * kHeight * sizeof(std::uint32_t);
static constexpr int kMaxStatus = 20;

#define LAGI_ALIGN(v, a) (((v) + ((a) - 1)) & ~((a) - 1))

extern "C" {
extern const unsigned char _binary_lagi_color_v_gxp_start[];
extern const unsigned char _binary_lagi_color_f_gxp_start[];
}

struct StatusLine {
    char text[78];
    std::uint32_t color;
};

static bool g_initialized = false;
static bool g_azelAlive = false;
static bool g_discAlive = false;
static bool g_debugVisible = true;
static StatusLine g_status[kMaxStatus]{};
static int g_statusCount = 0;

// Display/GXM resources.
static SceUID g_frameUid[2] = { -1, -1 };
static std::uint32_t* g_frameBuffer[2] = { nullptr, nullptr };
static SceGxmColorSurface g_colorSurface[2]{};
static SceGxmSyncObject* g_sync[2] = { nullptr, nullptr };
static int g_drawBuffer = 0;

static SceGxmContext* g_context = nullptr;
static SceGxmRenderTarget* g_renderTarget = nullptr;
static SceGxmDepthStencilSurface g_depthSurface{};

static SceUID g_vdmUid = -1;
static SceUID g_vertexRingUid = -1;
static SceUID g_fragmentRingUid = -1;
static SceUID g_fragmentUsseRingUid = -1;
static SceUID g_depthUid = -1;
static SceUID g_stencilUid = -1;

static void* g_contextHost = nullptr;
static void* g_vdmRing = nullptr;
static void* g_vertexRing = nullptr;
static void* g_fragmentRing = nullptr;
static void* g_fragmentUsseRing = nullptr;
static void* g_depthData = nullptr;
static void* g_stencilData = nullptr;

static SceGxmShaderPatcher* g_shaderPatcher = nullptr;
static SceUID g_patcherBufferUid = -1;
static SceUID g_patcherVertexUsseUid = -1;
static SceUID g_patcherFragmentUsseUid = -1;
static void* g_patcherBuffer = nullptr;
static void* g_patcherVertexUsse = nullptr;
static void* g_patcherFragmentUsse = nullptr;
static SceGxmShaderPatcherId g_vertexProgramId{};
static SceGxmShaderPatcherId g_fragmentProgramId{};
static SceGxmVertexProgram* g_vertexProgram = nullptr;
static SceGxmFragmentProgram* g_fragmentProgram = nullptr;
static const SceGxmProgramParameter* g_wvpParam = nullptr;

// Viewer resources.
static SceUID g_meshVertexUid = -1;
static SceUID g_meshIndexUid = -1;
static azel::DebugColorVertex* g_meshVertices = nullptr;
static std::uint16_t* g_meshIndices = nullptr;
static unsigned g_meshVertexCount = 0;
static bool g_viewerReady = false;
static float g_yaw = 0.0f;
static float g_pitch = 0.0f;
static int g_viewMode = 0;

static unsigned int alignedSize(SceKernelMemBlockType type, unsigned int size)
{
    if (type == SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW)
        return LAGI_ALIGN(size, 256 * 1024);
    return LAGI_ALIGN(size, 4 * 1024);
}

static void* gpuAlloc(SceKernelMemBlockType type, unsigned int size,
                      unsigned int attribs, SceUID* uid)
{
    const unsigned int bytes = alignedSize(type, size);
    *uid = sceKernelAllocMemBlock("LagiGxmMem", type, bytes, nullptr);
    if (*uid < 0)
        return nullptr;

    void* mem = nullptr;
    if (sceKernelGetMemBlockBase(*uid, &mem) < 0 || !mem) {
        sceKernelFreeMemBlock(*uid);
        *uid = -1;
        return nullptr;
    }

    if (sceGxmMapMemory(mem, bytes, attribs) < 0) {
        sceKernelFreeMemBlock(*uid);
        *uid = -1;
        return nullptr;
    }
    return mem;
}

static void gpuFree(SceUID& uid)
{
    if (uid < 0)
        return;
    void* mem = nullptr;
    if (sceKernelGetMemBlockBase(uid, &mem) >= 0 && mem)
        sceGxmUnmapMemory(mem);
    sceKernelFreeMemBlock(uid);
    uid = -1;
}

static void* vertexUsseAlloc(unsigned int size, SceUID* uid, unsigned int* offset)
{
    size = LAGI_ALIGN(size, 4096);
    *uid = sceKernelAllocMemBlock(
        "LagiVertexUsse", SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE, size, nullptr);
    if (*uid < 0)
        return nullptr;

    void* mem = nullptr;
    if (sceKernelGetMemBlockBase(*uid, &mem) < 0 || !mem)
        return nullptr;
    if (sceGxmMapVertexUsseMemory(mem, size, offset) < 0)
        return nullptr;
    return mem;
}

static void* fragmentUsseAlloc(unsigned int size, SceUID* uid, unsigned int* offset)
{
    size = LAGI_ALIGN(size, 4096);
    *uid = sceKernelAllocMemBlock(
        "LagiFragmentUsse", SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE, size, nullptr);
    if (*uid < 0)
        return nullptr;

    void* mem = nullptr;
    if (sceKernelGetMemBlockBase(*uid, &mem) < 0 || !mem)
        return nullptr;
    if (sceGxmMapFragmentUsseMemory(mem, size, offset) < 0)
        return nullptr;
    return mem;
}

static void* patcherHostAlloc(void*, unsigned int size) { return std::malloc(size); }
static void patcherHostFree(void*, void* mem) { std::free(mem); }

static void displayQueueCallback(const void*) {}

static void fill(std::uint32_t color)
{
    std::uint32_t* buffer = g_frameBuffer[g_drawBuffer];
    if (!buffer) return;
    for (int y = 0; y < kHeight; ++y) {
        std::uint32_t* row = buffer + y * kPitch;
        for (int x = 0; x < kWidth; ++x) row[x] = color;
    }
}

static const std::uint8_t* glyph(char c)
{
    static const std::uint8_t blank[7] = {0,0,0,0,0,0,0};
    static const std::uint8_t letters[26][7] = {
        {14,17,17,31,17,17,17},{30,17,17,30,17,17,30},{14,17,16,16,16,17,14},
        {30,17,17,17,17,17,30},{31,16,16,30,16,16,31},{31,16,16,30,16,16,16},
        {14,17,16,23,17,17,15},{17,17,17,31,17,17,17},{31,4,4,4,4,4,31},
        {7,2,2,2,18,18,12},{17,18,20,24,20,18,17},{16,16,16,16,16,16,31},
        {17,27,21,21,17,17,17},{17,25,21,19,17,17,17},{14,17,17,17,17,17,14},
        {30,17,17,30,16,16,16},{14,17,17,17,21,18,13},{30,17,17,30,20,18,17},
        {15,16,16,14,1,1,30},{31,4,4,4,4,4,4},{17,17,17,17,17,17,14},
        {17,17,17,17,17,10,4},{17,17,17,21,21,21,10},{17,17,10,4,10,17,17},
        {17,17,10,4,4,4,4},{31,1,2,4,8,16,31}
    };
    static const std::uint8_t digits[10][7] = {
        {14,17,19,21,25,17,14},{4,12,4,4,4,4,14},{14,17,1,2,4,8,31},
        {30,1,1,14,1,1,30},{2,6,10,18,31,2,2},{31,16,16,30,1,1,30},
        {14,16,16,30,17,17,14},{31,1,2,4,8,8,8},{14,17,17,14,17,17,14},
        {14,17,17,15,1,1,14}
    };
    static const std::uint8_t colon[7] = {0,4,4,0,4,4,0};
    static const std::uint8_t dash[7]  = {0,0,0,31,0,0,0};
    static const std::uint8_t slash[7] = {1,2,2,4,8,8,16};
    static const std::uint8_t dot[7]   = {0,0,0,0,0,6,6};
    static const std::uint8_t lbr[7]   = {14,8,8,8,8,8,14};
    static const std::uint8_t rbr[7]   = {14,2,2,2,2,2,14};

    c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    if (c >= 'A' && c <= 'Z') return letters[c - 'A'];
    if (c >= '0' && c <= '9') return digits[c - '0'];
    if (c == ':') return colon;
    if (c == '-') return dash;
    if (c == '/') return slash;
    if (c == '.') return dot;
    if (c == '[') return lbr;
    if (c == ']') return rbr;
    return blank;
}

static void drawChar(int x, int y, char c, std::uint32_t color, int scale = 2)
{
    const std::uint8_t* rows = glyph(c);
    for (int gy = 0; gy < 7; ++gy) {
        for (int gx = 0; gx < 5; ++gx) {
            if (!(rows[gy] & (1u << (4 - gx)))) continue;
            for (int sy = 0; sy < scale; ++sy) {
                const int py = y + gy * scale + sy;
                if (py < 0 || py >= kHeight) continue;
                for (int sx = 0; sx < scale; ++sx) {
                    const int px = x + gx * scale + sx;
                    if (px >= 0 && px < kWidth)
                        g_frameBuffer[g_drawBuffer][py * kPitch + px] = color;
                }
            }
        }
    }
}

static void drawText(int x, int y, const char* text, std::uint32_t color, int scale = 2)
{
    if (!text) return;
    const int advance = 6 * scale;
    for (const char* p = text; *p; ++p) {
        drawChar(x, y, *p, color, scale);
        x += advance;
        if (x > kWidth - advance) break;
    }
}

struct Mat4 { float m[16]; };

static Mat4 matIdentity()
{
    Mat4 r{};
    r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.0f;
    return r;
}

static Mat4 matMul(const Mat4& a, const Mat4& b)
{
    Mat4 r{};
    for (int row = 0; row < 4; ++row)
        for (int col = 0; col < 4; ++col)
            for (int k = 0; k < 4; ++k)
                r.m[row * 4 + col] += a.m[row * 4 + k] * b.m[k * 4 + col];
    return r;
}

static Mat4 matRotationX(float a)
{
    Mat4 r = matIdentity();
    const float c = std::cos(a), s = std::sin(a);
    r.m[5] = c; r.m[6] = s;
    r.m[9] = -s; r.m[10] = c;
    return r;
}

static Mat4 matRotationY(float a)
{
    Mat4 r = matIdentity();
    const float c = std::cos(a), s = std::sin(a);
    r.m[0] = c; r.m[2] = -s;
    r.m[8] = s; r.m[10] = c;
    return r;
}

static Mat4 matTranslation(float x, float y, float z)
{
    Mat4 r = matIdentity();
    r.m[12] = x; r.m[13] = y; r.m[14] = z;
    return r;
}

static Mat4 matPerspective(float fovDegrees, float aspect, float nearZ, float farZ)
{
    constexpr float kPi = 3.14159265358979323846f;
    const float halfHeight = nearZ * std::tan((fovDegrees * kPi / 180.0f) * 0.5f);
    const float halfWidth = halfHeight * aspect;
    const float left = -halfWidth, right = halfWidth;
    const float bottom = -halfHeight, top = halfHeight;

    Mat4 m{};
    m.m[0] = (2.0f * nearZ) / (right - left);
    m.m[5] = (2.0f * nearZ) / (top - bottom);
    m.m[8] = (right + left) / (right - left);
    m.m[9] = (top + bottom) / (top - bottom);
    m.m[10] = -(farZ + nearZ) / (farZ - nearZ);
    m.m[11] = -1.0f;
    m.m[14] = (-2.0f * farZ * nearZ) / (farZ - nearZ);
    return m;
}

static bool initGxmCore()
{
    SceGxmInitializeParams ip{};
    ip.flags = 0;
    ip.displayQueueMaxPendingCount = 1;
    ip.displayQueueCallback = displayQueueCallback;
    ip.displayQueueCallbackDataSize = 0;
    ip.parameterBufferSize = SCE_GXM_DEFAULT_PARAMETER_BUFFER_SIZE;
    if (sceGxmInitialize(&ip) < 0)
        return false;

    g_vdmRing = gpuAlloc(SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE,
        SCE_GXM_DEFAULT_VDM_RING_BUFFER_SIZE, SCE_GXM_MEMORY_ATTRIB_READ, &g_vdmUid);
    g_vertexRing = gpuAlloc(SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE,
        SCE_GXM_DEFAULT_VERTEX_RING_BUFFER_SIZE, SCE_GXM_MEMORY_ATTRIB_READ, &g_vertexRingUid);
    g_fragmentRing = gpuAlloc(SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE,
        SCE_GXM_DEFAULT_FRAGMENT_RING_BUFFER_SIZE, SCE_GXM_MEMORY_ATTRIB_READ, &g_fragmentRingUid);

    unsigned fragmentUsseOffset = 0;
    g_fragmentUsseRing = fragmentUsseAlloc(
        SCE_GXM_DEFAULT_FRAGMENT_USSE_RING_BUFFER_SIZE,
        &g_fragmentUsseRingUid, &fragmentUsseOffset);

    if (!g_vdmRing || !g_vertexRing || !g_fragmentRing || !g_fragmentUsseRing)
        return false;

    SceGxmContextParams cp{};
    g_contextHost = std::malloc(SCE_GXM_MINIMUM_CONTEXT_HOST_MEM_SIZE);
    cp.hostMem = g_contextHost;
    cp.hostMemSize = SCE_GXM_MINIMUM_CONTEXT_HOST_MEM_SIZE;
    cp.vdmRingBufferMem = g_vdmRing;
    cp.vdmRingBufferMemSize = SCE_GXM_DEFAULT_VDM_RING_BUFFER_SIZE;
    cp.vertexRingBufferMem = g_vertexRing;
    cp.vertexRingBufferMemSize = SCE_GXM_DEFAULT_VERTEX_RING_BUFFER_SIZE;
    cp.fragmentRingBufferMem = g_fragmentRing;
    cp.fragmentRingBufferMemSize = SCE_GXM_DEFAULT_FRAGMENT_RING_BUFFER_SIZE;
    cp.fragmentUsseRingBufferMem = g_fragmentUsseRing;
    cp.fragmentUsseRingBufferMemSize = SCE_GXM_DEFAULT_FRAGMENT_USSE_RING_BUFFER_SIZE;
    cp.fragmentUsseRingBufferOffset = fragmentUsseOffset;

    if (sceGxmCreateContext(&cp, &g_context) < 0)
        return false;

    SceGxmRenderTargetParams rp{};
    rp.flags = 0;
    rp.width = kWidth;
    rp.height = kHeight;
    rp.scenesPerFrame = 1;
    rp.multisampleMode = SCE_GXM_MULTISAMPLE_NONE;
    rp.multisampleLocations = 0;
    rp.driverMemBlock = -1;
    if (sceGxmCreateRenderTarget(&rp, &g_renderTarget) < 0)
        return false;

    return true;
}

static bool initSurfaces()
{
    for (int i = 0; i < 2; ++i) {
        g_frameBuffer[i] = static_cast<std::uint32_t*>(
            gpuAlloc(SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW,
                     static_cast<unsigned int>(kFrameBytes),
                     SCE_GXM_MEMORY_ATTRIB_READ | SCE_GXM_MEMORY_ATTRIB_WRITE,
                     &g_frameUid[i]));
        if (!g_frameBuffer[i])
            return false;

        std::memset(g_frameBuffer[i], 0, kFrameBytes);

        if (sceGxmColorSurfaceInit(
                &g_colorSurface[i],
                SCE_GXM_COLOR_FORMAT_A8B8G8R8,
                SCE_GXM_COLOR_SURFACE_LINEAR,
                SCE_GXM_COLOR_SURFACE_SCALE_NONE,
                SCE_GXM_OUTPUT_REGISTER_SIZE_32BIT,
                kWidth, kHeight, kPitch, g_frameBuffer[i]) < 0)
            return false;

        if (sceGxmSyncObjectCreate(&g_sync[i]) < 0)
            return false;
    }

    const unsigned alignedW = LAGI_ALIGN(kWidth, SCE_GXM_TILE_SIZEX);
    const unsigned alignedH = LAGI_ALIGN(kHeight, SCE_GXM_TILE_SIZEY);
    const unsigned samples = alignedW * alignedH;

    g_depthData = gpuAlloc(SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE,
                           4 * samples,
                           SCE_GXM_MEMORY_ATTRIB_READ | SCE_GXM_MEMORY_ATTRIB_WRITE,
                           &g_depthUid);
    g_stencilData = gpuAlloc(SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE,
                             4 * samples,
                             SCE_GXM_MEMORY_ATTRIB_READ | SCE_GXM_MEMORY_ATTRIB_WRITE,
                             &g_stencilUid);
    if (!g_depthData || !g_stencilData)
        return false;

    if (sceGxmDepthStencilSurfaceInit(
            &g_depthSurface,
            SCE_GXM_DEPTH_STENCIL_FORMAT_S8D24,
            SCE_GXM_DEPTH_STENCIL_SURFACE_TILED,
            alignedW,
            g_depthData,
            g_stencilData) < 0)
        return false;

    return true;
}

static bool initShaders()
{
    constexpr unsigned patcherBufferSize = 64 * 1024;
    constexpr unsigned patcherVertexUsseSize = 64 * 1024;
    constexpr unsigned patcherFragmentUsseSize = 64 * 1024;

    g_patcherBuffer = gpuAlloc(SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE,
        patcherBufferSize, SCE_GXM_MEMORY_ATTRIB_READ | SCE_GXM_MEMORY_ATTRIB_WRITE,
        &g_patcherBufferUid);

    unsigned vertexUsseOffset = 0;
    g_patcherVertexUsse = vertexUsseAlloc(
        patcherVertexUsseSize, &g_patcherVertexUsseUid, &vertexUsseOffset);

    unsigned fragmentUsseOffset = 0;
    g_patcherFragmentUsse = fragmentUsseAlloc(
        patcherFragmentUsseSize, &g_patcherFragmentUsseUid, &fragmentUsseOffset);

    if (!g_patcherBuffer || !g_patcherVertexUsse || !g_patcherFragmentUsse)
        return false;

    SceGxmShaderPatcherParams pp{};
    pp.userData = nullptr;
    pp.hostAllocCallback = patcherHostAlloc;
    pp.hostFreeCallback = patcherHostFree;
    pp.bufferAllocCallback = nullptr;
    pp.bufferFreeCallback = nullptr;
    pp.bufferMem = g_patcherBuffer;
    pp.bufferMemSize = patcherBufferSize;
    pp.vertexUsseAllocCallback = nullptr;
    pp.vertexUsseFreeCallback = nullptr;
    pp.vertexUsseMem = g_patcherVertexUsse;
    pp.vertexUsseMemSize = patcherVertexUsseSize;
    pp.vertexUsseOffset = vertexUsseOffset;
    pp.fragmentUsseAllocCallback = nullptr;
    pp.fragmentUsseFreeCallback = nullptr;
    pp.fragmentUsseMem = g_patcherFragmentUsse;
    pp.fragmentUsseMemSize = patcherFragmentUsseSize;
    pp.fragmentUsseOffset = fragmentUsseOffset;

    if (sceGxmShaderPatcherCreate(&pp, &g_shaderPatcher) < 0)
        return false;

    const SceGxmProgram* vp =
        reinterpret_cast<const SceGxmProgram*>(_binary_lagi_color_v_gxp_start);
    const SceGxmProgram* fp =
        reinterpret_cast<const SceGxmProgram*>(_binary_lagi_color_f_gxp_start);

    if (sceGxmProgramCheck(vp) < 0 || sceGxmProgramCheck(fp) < 0)
        return false;

    if (sceGxmShaderPatcherRegisterProgram(g_shaderPatcher, vp, &g_vertexProgramId) < 0 ||
        sceGxmShaderPatcherRegisterProgram(g_shaderPatcher, fp, &g_fragmentProgramId) < 0)
        return false;

    const SceGxmProgramParameter* pos =
        sceGxmProgramFindParameterByName(vp, "aPosition");
    const SceGxmProgramParameter* color =
        sceGxmProgramFindParameterByName(vp, "aColor");
    g_wvpParam = sceGxmProgramFindParameterByName(vp, "wvp");
    if (!pos || !color || !g_wvpParam)
        return false;

    SceGxmVertexAttribute attrs[2]{};
    attrs[0].streamIndex = 0;
    attrs[0].offset = 0;
    attrs[0].format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
    attrs[0].componentCount = 3;
    attrs[0].regIndex = sceGxmProgramParameterGetResourceIndex(pos);

    attrs[1].streamIndex = 0;
    attrs[1].offset = 12;
    attrs[1].format = SCE_GXM_ATTRIBUTE_FORMAT_U8N;
    attrs[1].componentCount = 4;
    attrs[1].regIndex = sceGxmProgramParameterGetResourceIndex(color);

    SceGxmVertexStream stream{};
    stream.stride = sizeof(azel::DebugColorVertex);
    stream.indexSource = SCE_GXM_INDEX_SOURCE_INDEX_16BIT;

    if (sceGxmShaderPatcherCreateVertexProgram(
            g_shaderPatcher, g_vertexProgramId,
            attrs, 2, &stream, 1, &g_vertexProgram) < 0)
        return false;

    if (sceGxmShaderPatcherCreateFragmentProgram(
            g_shaderPatcher, g_fragmentProgramId,
            SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4,
            SCE_GXM_MULTISAMPLE_NONE,
            nullptr,
            vp,
            &g_fragmentProgram) < 0)
        return false;

    return true;
}

bool init()
{
    if (!initGxmCore() || !initSurfaces() || !initShaders()) {
        shutdown();
        return false;
    }

    g_drawBuffer = 0;
    fill(0xFF181818u);
    g_drawBuffer = 1;
    fill(0xFF181818u);
    g_drawBuffer = 0;

    SceDisplayFrameBuf fb{};
    fb.size = sizeof(fb);
    fb.base = g_frameBuffer[0];
    fb.pitch = kPitch;
    fb.pixelformat = SCE_DISPLAY_PIXELFORMAT_A8B8G8R8;
    fb.width = kWidth;
    fb.height = kHeight;

    if (sceDisplaySetFrameBuf(&fb, SCE_DISPLAY_SETBUF_NEXTFRAME) < 0) {
        shutdown();
        return false;
    }

    sceDisplayWaitVblankStart();
    g_drawBuffer = 1;
    g_initialized = true;
    return true;
}

void shutdown()
{
    if (g_context)
        sceGxmFinish(g_context);

    if (g_shaderPatcher) {
        if (g_vertexProgram)
            sceGxmShaderPatcherReleaseVertexProgram(g_shaderPatcher, g_vertexProgram);
        if (g_fragmentProgram)
            sceGxmShaderPatcherReleaseFragmentProgram(g_shaderPatcher, g_fragmentProgram);
        sceGxmShaderPatcherUnregisterProgram(g_shaderPatcher, g_vertexProgramId);
        sceGxmShaderPatcherUnregisterProgram(g_shaderPatcher, g_fragmentProgramId);
        sceGxmShaderPatcherDestroy(g_shaderPatcher);
    }
    g_vertexProgram = nullptr;
    g_fragmentProgram = nullptr;
    g_shaderPatcher = nullptr;

    gpuFree(g_meshVertexUid);
    gpuFree(g_meshIndexUid);
    g_meshVertices = nullptr;
    g_meshIndices = nullptr;

    for (int i = 0; i < 2; ++i) {
        if (g_sync[i]) {
            sceGxmSyncObjectDestroy(g_sync[i]);
            g_sync[i] = nullptr;
        }
        gpuFree(g_frameUid[i]);
        g_frameBuffer[i] = nullptr;
    }

    gpuFree(g_depthUid);
    gpuFree(g_stencilUid);
    gpuFree(g_patcherBufferUid);
    gpuFree(g_vdmUid);
    gpuFree(g_vertexRingUid);
    gpuFree(g_fragmentRingUid);

    if (g_patcherVertexUsseUid >= 0) {
        void* mem = nullptr;
        if (sceKernelGetMemBlockBase(g_patcherVertexUsseUid, &mem) >= 0 && mem)
            sceGxmUnmapVertexUsseMemory(mem);
        sceKernelFreeMemBlock(g_patcherVertexUsseUid);
        g_patcherVertexUsseUid = -1;
    }
    if (g_patcherFragmentUsseUid >= 0) {
        void* mem = nullptr;
        if (sceKernelGetMemBlockBase(g_patcherFragmentUsseUid, &mem) >= 0 && mem)
            sceGxmUnmapFragmentUsseMemory(mem);
        sceKernelFreeMemBlock(g_patcherFragmentUsseUid);
        g_patcherFragmentUsseUid = -1;
    }
    if (g_fragmentUsseRingUid >= 0) {
        void* mem = nullptr;
        if (sceKernelGetMemBlockBase(g_fragmentUsseRingUid, &mem) >= 0 && mem)
            sceGxmUnmapFragmentUsseMemory(mem);
        sceKernelFreeMemBlock(g_fragmentUsseRingUid);
        g_fragmentUsseRingUid = -1;
    }

    if (g_renderTarget) {
        sceGxmDestroyRenderTarget(g_renderTarget);
        g_renderTarget = nullptr;
    }
    if (g_context) {
        sceGxmDestroyContext(g_context);
        g_context = nullptr;
    }
    std::free(g_contextHost);
    g_contextHost = nullptr;

    if (g_initialized)
        sceGxmTerminate();

    g_initialized = false;
}

void status(const char* text, unsigned int color)
{
    if (!text || g_statusCount >= kMaxStatus) return;
    StatusLine& line = g_status[g_statusCount++];
    std::strncpy(line.text, text, sizeof(line.text) - 1);
    line.text[sizeof(line.text) - 1] = 0;
    line.color = color;
}

void failure(const char* text) { status(text, 0xFF3030FFu); }
void set_azel_alive(bool alive) { g_azelAlive = alive; }
void set_disc_alive(bool alive) { g_discAlive = alive; }

void toggle_debug_console() { g_debugVisible = !g_debugVisible; }
bool debug_console_visible() { return g_debugVisible; }

bool load_basic_wing_viewer()
{
    azel::BasicWingDebugMesh mesh;
    if (!azel::build_basic_wing_debug_mesh(mesh) || mesh.vertices.empty())
        return false;

    if (mesh.vertices.size() > 65535)
        return false;

    float minX = mesh.vertices[0].x, maxX = mesh.vertices[0].x;
    float minY = mesh.vertices[0].y, maxY = mesh.vertices[0].y;
    float minZ = mesh.vertices[0].z, maxZ = mesh.vertices[0].z;
    for (const auto& v : mesh.vertices) {
        minX = std::min(minX, v.x); maxX = std::max(maxX, v.x);
        minY = std::min(minY, v.y); maxY = std::max(maxY, v.y);
        minZ = std::min(minZ, v.z); maxZ = std::max(maxZ, v.z);
    }

    const float cx = (minX + maxX) * 0.5f;
    const float cy = (minY + maxY) * 0.5f;
    const float cz = (minZ + maxZ) * 0.5f;
    const float ex = std::max(std::fabs(maxX - cx), 0.001f);
    const float ey = std::max(std::fabs(maxY - cy), 0.001f);
    const float ez = std::max(std::fabs(maxZ - cz), 0.001f);
    const float scale = 1.25f / std::max(ex, std::max(ey, ez));

    for (auto& v : mesh.vertices) {
        v.x = (v.x - cx) * scale;
        v.y = (v.y - cy) * scale;
        v.z = (v.z - cz) * scale;
    }

    gpuFree(g_meshVertexUid);
    gpuFree(g_meshIndexUid);

    const unsigned vertexBytes =
        static_cast<unsigned>(mesh.vertices.size() * sizeof(azel::DebugColorVertex));
    const unsigned indexBytes =
        static_cast<unsigned>(mesh.vertices.size() * sizeof(std::uint16_t));

    g_meshVertices = static_cast<azel::DebugColorVertex*>(
        gpuAlloc(SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE,
                 vertexBytes, SCE_GXM_MEMORY_ATTRIB_READ, &g_meshVertexUid));
    g_meshIndices = static_cast<std::uint16_t*>(
        gpuAlloc(SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE,
                 indexBytes, SCE_GXM_MEMORY_ATTRIB_READ, &g_meshIndexUid));
    if (!g_meshVertices || !g_meshIndices)
        return false;

    std::memcpy(g_meshVertices, mesh.vertices.data(), vertexBytes);
    for (unsigned i = 0; i < mesh.vertices.size(); ++i)
        g_meshIndices[i] = static_cast<std::uint16_t>(i);

    g_meshVertexCount = static_cast<unsigned>(mesh.vertices.size());
    g_yaw = 0.0f;
    g_pitch = 0.0f;
    g_viewMode = 0;
    g_viewerReady = true;
    return true;
}

static void drawStatusConsole()
{
    fill(0xFF181818u);
    drawText(32, 24, "LAGI - PDS VITA RUNTIME", 0xFFFFFFFFu, 3);
    drawText(32, 58, "BOOT / INTEGRATION STATUS", 0xFFB0B0B0u, 2);

    int y = 92;
    for (int i = 0; i < g_statusCount; ++i) {
        drawText(40, y, g_status[i].text, g_status[i].color, 2);
        y += 22;
    }

    if (g_azelAlive)
        drawText(40, y + 8, "TASK LOOP: ACTIVE", 0xFF30E030u, 2);

    drawText(40, kHeight - 34, "SELECT: 3D VIEW", 0xFFB0B0B0u, 2);
}

static void drawViewer()
{
    fill(0xFF101014u);

    if (!g_viewerReady || !g_context)
        return;

    g_yaw += input::analog_x() * 0.035f;
    g_pitch += input::analog_y() * 0.035f;
    g_pitch = std::max(-1.45f, std::min(1.45f, g_pitch));

    if (input::reset_view_pressed()) {
        g_yaw = 0.0f;
        g_pitch = 0.0f;
    }
    if (input::prev_mode_pressed())
        g_viewMode = (g_viewMode + 1) % 2;
    if (input::next_mode_pressed())
        g_viewMode = (g_viewMode + 1) % 2;

    // S8D24 starts at maximum depth.
    const unsigned alignedW = LAGI_ALIGN(kWidth, SCE_GXM_TILE_SIZEX);
    const unsigned alignedH = LAGI_ALIGN(kHeight, SCE_GXM_TILE_SIZEY);
    std::memset(g_depthData, 0xFF, alignedW * alignedH * 4);
    std::memset(g_stencilData, 0, alignedW * alignedH * 4);

    if (sceGxmBeginScene(
            g_context, 0, g_renderTarget, nullptr, nullptr,
            g_sync[g_drawBuffer], &g_colorSurface[g_drawBuffer],
            &g_depthSurface) < 0)
        return;

    sceGxmSetVertexProgram(g_context, g_vertexProgram);
    sceGxmSetFragmentProgram(g_context, g_fragmentProgram);
    sceGxmSetCullMode(g_context, SCE_GXM_CULL_NONE);
    sceGxmSetDefaultRegionClipAndViewport(g_context);
    sceGxmSetFrontDepthFunc(g_context, SCE_GXM_DEPTH_FUNC_LESS_EQUAL);
    sceGxmSetBackDepthFunc(g_context, SCE_GXM_DEPTH_FUNC_LESS_EQUAL);
    sceGxmSetFrontDepthWriteEnable(g_context, SCE_GXM_DEPTH_WRITE_ENABLED);
    sceGxmSetBackDepthWriteEnable(g_context, SCE_GXM_DEPTH_WRITE_ENABLED);

    const SceGxmPolygonMode polygonMode =
        g_viewMode == 0 ? SCE_GXM_POLYGON_MODE_TRIANGLE_FILL : SCE_GXM_POLYGON_MODE_LINE;
    sceGxmSetFrontPolygonMode(g_context, polygonMode);
    sceGxmSetBackPolygonMode(g_context, polygonMode);

    const Mat4 rx = matRotationX(g_pitch);
    const Mat4 ry = matRotationY(g_yaw);
    const Mat4 model = matMul(rx, ry);
    const Mat4 view = matTranslation(0.0f, 0.0f, -3.2f);
    const Mat4 projection = matPerspective(
        50.0f, static_cast<float>(kWidth) / static_cast<float>(kHeight), 0.1f, 20.0f);
    const Mat4 mv = matMul(model, view);
    const Mat4 wvp = matMul(mv, projection);

    void* uniformBuffer = nullptr;
    sceGxmReserveVertexDefaultUniformBuffer(g_context, &uniformBuffer);
    sceGxmSetUniformDataF(uniformBuffer, g_wvpParam, 0, 16, wvp.m);

    sceGxmSetVertexStream(g_context, 0, g_meshVertices);
    sceGxmDraw(g_context, SCE_GXM_PRIMITIVE_TRIANGLES,
               SCE_GXM_INDEX_FORMAT_U16, g_meshIndices, g_meshVertexCount);

    sceGxmEndScene(g_context, nullptr, nullptr);
    sceGxmFinish(g_context);

    // Minimal viewer legend is CPU written only after GXM has finished.
    drawText(24, 18, "BASIC WING - GXM DEBUG VIEW", 0xFFFFFFFFu, 2);
    drawText(24, 42, g_viewMode == 0 ? "MODE: PER-POLYGON COLOR" : "MODE: WIREFRAME",
             0xFFB0B0B0u, 2);
    drawText(24, kHeight - 56, "LEFT STICK: ROTATE   TRIANGLE: RESET", 0xFFB0B0B0u, 2);
    drawText(24, kHeight - 32, "L/R: MODE   SELECT: STATUS", 0xFFB0B0B0u, 2);
}

void begin_frame()
{
    if (g_debugVisible)
        drawStatusConsole();
    else
        drawViewer();
}

void end_frame()
{
    SceDisplayFrameBuf fb{};
    fb.size = sizeof(fb);
    fb.base = g_frameBuffer[g_drawBuffer];
    fb.pitch = kPitch;
    fb.pixelformat = SCE_DISPLAY_PIXELFORMAT_A8B8G8R8;
    fb.width = kWidth;
    fb.height = kHeight;

    sceDisplaySetFrameBuf(&fb, SCE_DISPLAY_SETBUF_NEXTFRAME);
    sceDisplayWaitVblankStart();
    g_drawBuffer ^= 1;
}

} // namespace lagi::platform::renderer

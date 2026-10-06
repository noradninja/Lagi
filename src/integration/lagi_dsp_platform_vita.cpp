#include "lagi_dsp_platform.h"
#include "lagi/platform.h"
#include <psp2/kernel/sysmem.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <cstdarg>
#include <cstdio>
#include <cstring>

namespace {
bool initialized = false, vmReady = false;
int mode = LAGI_DSP_PREDECODED;
SceUID pool = -1;
void *poolBase = nullptr;
constexpr unsigned slotBytes = 65536, slots = 4;
constexpr unsigned vmBytes = 1024u * 1024u;
struct CaptureKey { unsigned steps; uint32_t hash; uint16_t words[512]; };
CaptureKey captured[64]{};
unsigned captureCount = 0;
int result(const char *operation, int status) {
    lagi_dsp_log("[LagiDSPVM] op=%s status=%d hex=%08X\n", operation, status, static_cast<unsigned>(status));
    return status;
}
bool probe() {
    pool = result("alloc", sceKernelAllocMemBlockForVM("LagiDSPCode", vmBytes));
    if (pool < 0) return false;
    poolBase = nullptr;
    bool opened = false, ok = false;
    if (result("base", sceKernelGetMemBlockBase(pool, &poolBase)) >= 0 && poolBase &&
        result("open", sceKernelOpenVMDomain()) >= 0) {
        opened = true;
        /* ARM AAPCS leaf: mov r0,#42; bx lr. Domain open permits writing;
         * close then sync before calling, following the Vita dynarec example. */
        const uint32_t code[] = { 0xe3a0002au, 0xe12fff1eu };
        std::memcpy(poolBase, code, sizeof(code));
        if (result("close", sceKernelCloseVMDomain()) >= 0) {
            opened = false;
            if (result("sync", sceKernelSyncVMDomain(pool, poolBase, 4096)) >= 0) {
                const int value = reinterpret_cast<int (*)(void)>(poolBase)();
                result("execute", value);
                ok = value == 42;
            }
        }
    }
    if (opened && result("cleanup-close", sceKernelCloseVMDomain()) < 0) ok = false;
    if (!ok || (mode != LAGI_DSP_AUTO && mode != LAGI_DSP_ARM)) {
        if (result("free", sceKernelFreeMemBlock(pool)) < 0) ok = false;
        else { pool = -1; poolBase = nullptr; }
    }
    return ok;
}
}

extern "C" void lagi_dsp_log(const char *format, ...) {
    char buffer[1024]; va_list args; va_start(args, format);
    std::vsnprintf(buffer, sizeof(buffer), format, args); va_end(args);
    lagi::platform::logging::writef("%s", buffer);
}
extern "C" int lagi_dsp_platform_init() {
    if (initialized) return mode;
    initialized = true;
    char config[32]{};
    const SceUID fd = sceIoOpen("ux0:data/lagi/dsp_backend.txt", SCE_O_RDONLY, 0);
    if (fd >= 0) { sceIoRead(fd, config, sizeof(config)-1); sceIoClose(fd); }
    config[std::strcspn(config, "\r\n \t")] = 0;
    if (!std::strcmp(config, "reference")) mode = LAGI_DSP_REFERENCE;
    else if (!std::strcmp(config, "auto")) mode = LAGI_DSP_AUTO;
    else if (!std::strcmp(config, "aot")) mode = LAGI_DSP_AOT;
    else if (!std::strcmp(config, "arm")) mode = LAGI_DSP_ARM;
    else if (config[0] && std::strcmp(config, "predecoded"))
        lagi_dsp_log("[LagiDSP] invalid backend setting; using predecoded\n");
    vmReady = probe();
    lagi_dsp_log("[LagiDSP] configured=%d vmReady=%d capture=on\n", mode, vmReady);
    return mode;
}
extern "C" int lagi_dsp_vm_available() { return vmReady; }
extern "C" void lagi_dsp_capture(const uint16_t *words, unsigned steps, uint32_t hash) {
    if (steps > 128) return;
    for (unsigned i=0; i<captureCount; ++i)
        if (captured[i].hash == hash && captured[i].steps == steps &&
            !std::memcmp(captured[i].words, words, steps*8)) return;
    /* When the RAM dedup cache fills, persistent full-file comparison still
     * captures new programs. Coverage is not limited to 64 programs. */
    if (sceIoMkdir("ux0:data/lagi/dsp_programs", 0777) < 0) {
        /* Existing directory is expected; individual open results are checked. */
    }
    uint32_t header[4] = { 0x5053444cu, 1, steps, hash }; // LDSP, little endian
    unsigned char blob[1040]{};
    std::memcpy(blob, header, 16);
    std::memcpy(blob+16, words, steps*8);
    const unsigned bytes = 16 + steps*8;
    bool saved = false;
    for (unsigned collision=0; collision<64 && !saved; ++collision) {
        char path[128]; std::snprintf(path, sizeof(path), "ux0:data/lagi/dsp_programs/%08X-%u-%u.ldsp", hash, steps, collision);
        SceUID fd = sceIoOpen(path, SCE_O_RDONLY, 0);
        if (fd >= 0) {
            unsigned char previous[1041]{};
            const int got = sceIoRead(fd, previous, sizeof(previous)); sceIoClose(fd);
            if (got == static_cast<int>(bytes) && !std::memcmp(previous, blob, bytes)) saved = true;
            else continue; // Never overwrite a hash collision or corrupt capture.
        } else {
            fd = sceIoOpen(path, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_EXCL, 0777);
            if (fd < 0) { lagi_dsp_log("[LagiDSPCapture] hash=%08X open=%d\n", hash, fd); break; }
            const int written = sceIoWrite(fd, blob, bytes);
            const int closed = sceIoClose(fd);
            saved = written == static_cast<int>(bytes) && closed >= 0;
            if (!saved) { sceIoRemove(path); lagi_dsp_log("[LagiDSPCapture] hash=%08X write=%d close=%d\n", hash, written, closed); break; }
            lagi_dsp_log("[LagiDSPCapture] hash=%08X steps=%u bytes=%u path=%s\n", hash, steps, bytes, path);
        }
    }
    if(!saved) lagi_dsp_log("[LagiDSPCapture] hash=%08X steps=%u reason=not-saved\n",hash,steps);
    if (saved && captureCount < 64) {
        CaptureKey &key = captured[captureCount++]; key.steps=steps; key.hash=hash;
        std::memcpy(key.words, words, steps*8);
    }
}
extern "C" void *lagi_dsp_vm_publish(unsigned slot, const uint32_t *code, unsigned bytes) {
    if (!vmReady || pool<0 || !code || slot >= slots || !bytes || (bytes&3) || bytes > slotBytes) return nullptr;
    auto *base = static_cast<unsigned char *>(poolBase) + slot*slotBytes;
    if (result("publish-open", sceKernelOpenVMDomain()) < 0) { vmReady=false; return nullptr; }
    std::memcpy(base, code, bytes);
    const int closed = result("publish-close", sceKernelCloseVMDomain());
    if (closed < 0) { result("publish-cleanup-close", sceKernelCloseVMDomain()); vmReady=false; return nullptr; }
    if (result("publish-sync", sceKernelSyncVMDomain(pool, base, slotBytes)) < 0) { vmReady=false; return nullptr; }
    return base;
}
extern "C" void lagi_dsp_platform_shutdown() {
    lagi_dsp_release_native();
    if (pool >= 0) { result("pool-free", sceKernelFreeMemBlock(pool)); pool=-1; poolBase=nullptr; }
    vmReady=false; initialized=false; captureCount=0;
}

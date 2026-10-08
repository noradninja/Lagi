#include "lagi/platform.h"
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <psp2/kernel/threadmgr.h>
#include <atomic>
#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cstring>

namespace lagi::platform::logging {
static SceUID g_logFd = -1, g_mutex = -1, g_wake = -1, g_worker = -1;
static std::atomic<int> g_lastError{0};
static std::atomic<bool> g_running{false}, g_async{false};
static constexpr const char* kLogDir = "ux0:data/lagi";
static constexpr const char* kLogPath = "ux0:data/lagi/lagi.log";
static char g_queue[256 * 1024];
static size_t g_read = 0, g_used = 0;
static unsigned g_dropped = 0;

static void writeRaw(const char* text, size_t length)
{
    while (g_logFd >= 0 && length) {
        const int result = sceIoWrite(g_logFd, text, length);
        if (result <= 0) {
            g_lastError.store(result < 0 ? result : -1);
            break;
        }
        text += result;
        length -= result;
    }
}

static int drainLog(SceSize, void*)
{
    char batch[16 * 1024];
    for (;;) {
        sceKernelWaitSema(g_wake, 1, nullptr);
        for (;;) {
            sceKernelWaitSema(g_mutex, 1, nullptr);
            const size_t count = std::min(g_used, sizeof(batch));
            const size_t first = std::min(count, sizeof(g_queue) - g_read);
            std::memcpy(batch, g_queue + g_read, first);
            std::memcpy(batch + first, g_queue, count - first);
            g_read = (g_read + count) % sizeof(g_queue);
            g_used -= count;
            const unsigned dropped = g_dropped;
            g_dropped = 0;
            const bool running = g_running.load();
            sceKernelSignalSema(g_mutex, 1);
            if (count) {
                writeRaw(batch, count);
                std::fwrite(batch, 1, count, stdout);
            }
            if (dropped) {
                char warning[96];
                const int length = std::snprintf(warning, sizeof(warning),
                    "[Log] queue full: dropped %u messages\n", dropped);
                writeRaw(warning, static_cast<size_t>(length));
            }
            if (!count) {
                if (!running) return 0;
                break;
            }
        }
    }
}

bool init()
{
    if (g_logFd >= 0) return true;
    g_lastError.store(0);
    sceIoMkdir(kLogDir, 0777);
    g_logFd = sceIoOpen(kLogPath, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
    if (g_logFd < 0) {
        g_lastError.store(g_logFd);
        return false;
    }
    const char header[] = "Lagi runtime log\n================\n";
    writeRaw(header, sizeof(header) - 1);
    g_read = g_used = 0;
    g_dropped = 0;
    g_mutex = sceKernelCreateSema("LagiLogMutex", 0, 1, 1, nullptr);
    g_wake = sceKernelCreateSema("LagiLogWake", 0, 0, 1, nullptr);
    if (g_mutex >= 0 && g_wake >= 0)
        g_worker = sceKernelCreateThread("LagiLogWriter", drainLog, 0x10000110,
                                        64 * 1024, 0, 0, nullptr);
    g_running.store(true);
    if (g_worker >= 0 && sceKernelStartThread(g_worker, 0, nullptr) >= 0) {
        g_async.store(true);
    } else {
        g_running.store(false);
        if (g_worker >= 0) sceKernelDeleteThread(g_worker);
        if (g_wake >= 0) sceKernelDeleteSema(g_wake);
        if (g_mutex >= 0) sceKernelDeleteSema(g_mutex);
        g_worker = g_wake = g_mutex = -1;
    }
    writef("[Log] persistent log: %s async=%u\n", kLogPath, g_async.load() ? 1u : 0u);
    return true;
}

void writef(const char* format, ...)
{
    if (!format) return;
    char buffer[1024];
    va_list args;
    va_start(args, format);
    const int written = std::vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    if (written <= 0) return;
    const size_t length = std::min(static_cast<size_t>(written), sizeof(buffer) - 1);
    if (!g_async.load()) {
        writeRaw(buffer, length);
        std::fwrite(buffer, 1, length, stdout);
        return;
    }
    // Only memory copies run on producers. Storage and stdout stay on the worker.
    sceKernelWaitSema(g_mutex, 1, nullptr);
    if (length <= sizeof(g_queue) - g_used) {
        const size_t tail = (g_read + g_used) % sizeof(g_queue);
        const size_t first = std::min(length, sizeof(g_queue) - tail);
        std::memcpy(g_queue + tail, buffer, first);
        std::memcpy(g_queue, buffer + first, length - first);
        g_used += length;
    } else {
        ++g_dropped;
    }
    sceKernelSignalSema(g_mutex, 1);
    // A full binary wake semaphore means the worker is already scheduled.
    sceKernelSignalSema(g_wake, 1);
}

void shutdown()
{
    if (g_logFd < 0) return;
    writef("[Log] shutdown\n");
    if (g_async.load()) {
        g_running.store(false);
        sceKernelSignalSema(g_wake, 1);
        sceKernelWaitThreadEnd(g_worker, nullptr, nullptr);
        g_async.store(false);
        sceKernelDeleteThread(g_worker);
        sceKernelDeleteSema(g_wake);
        sceKernelDeleteSema(g_mutex);
        g_worker = g_wake = g_mutex = -1;
    }
    sceIoClose(g_logFd);
    g_logFd = -1;
}

const char* path() { return kLogPath; }
bool ready() { return g_logFd >= 0; }
int last_error() { return g_lastError.load(); }
} // namespace lagi::platform::logging

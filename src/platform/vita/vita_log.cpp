#include "lagi/platform.h"

#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>

#include <cstdarg>
#include <cstdio>
#include <cstring>

namespace lagi::platform::logging {

static SceUID g_logFd = -1;
static int g_lastError = 0;
static constexpr const char* kLogDir = "ux0:data/lagi";
static constexpr const char* kLogPath = "ux0:data/lagi/lagi.log";

static void writeRaw(const char* text)
{
    if (g_logFd < 0 || !text)
        return;

    const int length = static_cast<int>(std::strlen(text));
    if (length <= 0)
        return;

    const int result = sceIoWrite(g_logFd, text, length);
    if (result < 0)
        g_lastError = result;
}

bool init()
{
    if (g_logFd >= 0)
        return true;

    g_lastError = 0;

    // Ensure the data directory exists. An "already exists" result is harmless.
    sceIoMkdir(kLogDir, 0777);

    g_logFd = sceIoOpen(
        kLogPath,
        SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC,
        0777);

    if (g_logFd < 0) {
        g_lastError = g_logFd;
        std::printf("[Log] sceIoOpen failed for %s: 0x%08X\n",
                    kLogPath, static_cast<unsigned int>(g_lastError));
        return false;
    }

    writeRaw("Lagi runtime log\n");
    writeRaw("================\n");

    std::printf("[Log] persistent log: %s\n", kLogPath);
    return true;
}

void shutdown()
{
    if (g_logFd < 0)
        return;

    writeRaw("[Log] shutdown\n");
    sceIoClose(g_logFd);
    g_logFd = -1;
}

void writef(const char* format, ...)
{
    if (!format)
        return;

    char buffer[1024];

    va_list args;
    va_start(args, format);

    va_list stdoutArgs;
    va_copy(stdoutArgs, args);
    std::vprintf(format, stdoutArgs);
    va_end(stdoutArgs);

    const int written = std::vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    if (written <= 0 || g_logFd < 0)
        return;

    buffer[sizeof(buffer) - 1] = '\0';
    writeRaw(buffer);
}

const char* path()
{
    return kLogPath;
}

bool ready()
{
    return g_logFd >= 0;
}

int last_error()
{
    return g_lastError;
}

} // namespace lagi::platform::logging

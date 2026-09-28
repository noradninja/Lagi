#include "lagi/platform.h"

#include <cstdarg>
#include <cstdio>

namespace lagi::platform::logging {

static FILE* g_logFile = nullptr;
static constexpr const char* kLogPath = "ux0:data/lagi/lagi.log";

bool init()
{
    if (g_logFile)
        return true;

    // Truncate on each launch so every test run has one self-contained log.
    g_logFile = std::fopen(kLogPath, "w");
    if (!g_logFile) {
        std::printf("[Log] failed to open %s\n", kLogPath);
        return false;
    }

    std::fprintf(g_logFile, "Lagi runtime log\n");
    std::fprintf(g_logFile, "================\n");
    std::fflush(g_logFile);

    std::printf("[Log] persistent log: %s\n", kLogPath);
    return true;
}

void shutdown()
{
    if (!g_logFile)
        return;

    std::fprintf(g_logFile, "[Log] shutdown\n");
    std::fflush(g_logFile);
    std::fclose(g_logFile);
    g_logFile = nullptr;
}

void writef(const char* format, ...)
{
    if (!format)
        return;

    va_list args;
    va_start(args, format);

    va_list stdoutArgs;
    va_copy(stdoutArgs, args);
    std::vprintf(format, stdoutArgs);
    va_end(stdoutArgs);

    if (g_logFile) {
        std::vfprintf(g_logFile, format, args);
        std::fflush(g_logFile);
    }

    va_end(args);
}

const char* path()
{
    return kLogPath;
}

} // namespace lagi::platform::logging

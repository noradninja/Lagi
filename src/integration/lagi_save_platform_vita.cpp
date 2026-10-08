#include "lagi/azel_save_platform.h"

#include "lagi/platform.h"

#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>

#include <array>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace {

constexpr const char* kSaveRoot = "ux0:data/lagi/save";
constexpr const char* kInternalSaveRoot = "ux0:data/lagi/save/0";
constexpr std::uint32_t kSaveVersion = 0x00010000u;
constexpr std::size_t kMaximumSaveBytes = 0x2000u;
constexpr std::array<const char*, 3> kSlotNames = {
    "PANDRA_3_01", "PANDRA_3_02", "PANDRA_3_03"
};
std::atomic<bool> g_inGameLoadPending{false};

bool isInternalDevice(int deviceId)
{
    return deviceId == 0;
}

bool isKnownSlot(const char* fileName)
{
    if (!fileName)
        return false;
    for (const char* slot : kSlotNames) {
        if (std::strcmp(fileName, slot) == 0)
            return true;
    }
    return false;
}

bool buildPath(
    char* output,
    std::size_t outputSize,
    int deviceId,
    const char* fileName,
    const char* suffix = "")
{
    if (!output || outputSize == 0 || !isInternalDevice(deviceId) ||
        !isKnownSlot(fileName))
        return false;
    const int length = std::snprintf(
        output, outputSize, "%s/%s%s", kInternalSaveRoot, fileName, suffix);
    return length > 0 && static_cast<std::size_t>(length) < outputSize;
}

void ensureSaveDirectories()
{
    sceIoMkdir("ux0:data/lagi", 0777);
    sceIoMkdir(kSaveRoot, 0777);
    sceIoMkdir(kInternalSaveRoot, 0777);
}

bool writeAll(SceUID fd, const std::uint8_t* bytes, std::size_t size)
{
    std::size_t written = 0;
    while (written < size) {
        const int result = sceIoWrite(fd, bytes + written, size - written);
        if (result <= 0)
            return false;
        written += static_cast<std::size_t>(result);
    }
    return true;
}

bool readAll(SceUID fd, std::uint8_t* bytes, std::size_t size)
{
    std::size_t read = 0;
    while (read < size) {
        const int result = sceIoRead(fd, bytes + read, size - read);
        if (result <= 0)
            return false;
        read += static_cast<std::size_t>(result);
    }
    std::uint8_t extra = 0;
    return sceIoRead(fd, &extra, 1) == 0;
}

std::uint32_t readU32(const std::uint8_t* bytes)
{
    std::uint32_t value = 0;
    std::memcpy(&value, bytes, sizeof(value));
    return value;
}

std::uint32_t computeChecksum(const std::uint8_t* bytes, std::size_t size)
{
    std::uint32_t sum = 0;
    for (std::size_t index = sizeof(std::uint32_t); index < size; ++index)
        sum += static_cast<std::uint32_t>(index - 3u) * bytes[index];
    return sum;
}

bool validSaveFile(int deviceId, const char* fileName)
{
    char path[160]{};
    if (!buildPath(path, sizeof(path), deviceId, fileName))
        return false;

    SceIoStat stat{};
    if (sceIoGetstat(path, &stat) < 0 || stat.st_size < 8 ||
        static_cast<std::uint64_t>(stat.st_size) > kMaximumSaveBytes)
        return false;

    std::array<std::uint8_t, kMaximumSaveBytes> bytes{};
    const SceUID fd = sceIoOpen(path, SCE_O_RDONLY, 0);
    if (fd < 0)
        return false;
    const bool read = readAll(
        fd, bytes.data(), static_cast<std::size_t>(stat.st_size));
    sceIoClose(fd);
    if (!read)
        return false;

    return readU32(bytes.data() + 4) == kSaveVersion &&
        readU32(bytes.data()) ==
            computeChecksum(bytes.data(), static_cast<std::size_t>(stat.st_size));
}

} // namespace

extern "C" int lagiAzelSaveDeviceAvailable(int deviceId)
{
    return isInternalDevice(deviceId) ? 1 : 0;
}

extern "C" int lagiAzelSaveFreeSlots(int deviceId)
{
    if (!isInternalDevice(deviceId))
        return 0;
    int freeSlots = 0;
    for (const char* slot : kSlotNames) {
        if (!validSaveFile(deviceId, slot))
            ++freeSlots;
    }
    return freeSlots;
}

extern "C" int lagiAzelSaveHasAnyValidSlot()
{
    unsigned int validMask = 0;
    unsigned int index = 0;
    for (const char* slot : kSlotNames) {
        if (validSaveFile(0, slot))
            validMask |= 1u << index;
        ++index;
    }
    lagi::platform::logging::writef(
        "[SavePlatform] title slots validMask=%u\n", validMask);
    return validMask != 0u ? 1 : 0;
}

extern "C" int lagiAzelSaveRead(
    int deviceId,
    const char* fileName,
    void* destination,
    std::size_t size)
{
    if (!destination || size < 8 || size > kMaximumSaveBytes)
        return -1;
    char path[160]{};
    if (!buildPath(path, sizeof(path), deviceId, fileName))
        return -1;

    const SceUID fd = sceIoOpen(path, SCE_O_RDONLY, 0);
    if (fd < 0)
        return -1;
    const bool read = readAll(
        fd, static_cast<std::uint8_t*>(destination), size);
    const int closeResult = sceIoClose(fd);
    if (!read || closeResult < 0) {
        lagi::platform::logging::writef(
            "[SavePlatform] read failed device=%d slot=%s size=%u\n",
            deviceId, fileName, static_cast<unsigned>(size));
        return -1;
    }

    lagi::platform::logging::writef(
        "[SavePlatform] read device=%d slot=%s size=%u\n",
        deviceId, fileName, static_cast<unsigned>(size));
    return 0;
}

extern "C" int lagiAzelSaveWrite(
    int deviceId,
    const char* fileName,
    const void* source,
    std::size_t size)
{
    if (!source || size < 8 || size > kMaximumSaveBytes)
        return -1;
    char path[160]{}, temporaryPath[160]{}, backupPath[160]{};
    if (!buildPath(path, sizeof(path), deviceId, fileName) ||
        !buildPath(temporaryPath, sizeof(temporaryPath), deviceId, fileName, ".tmp") ||
        !buildPath(backupPath, sizeof(backupPath), deviceId, fileName, ".bak"))
        return -1;

    ensureSaveDirectories();
    sceIoRemove(temporaryPath);
    const SceUID fd = sceIoOpen(
        temporaryPath, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0666);
    if (fd < 0)
        return -1;
    const bool written = writeAll(
        fd, static_cast<const std::uint8_t*>(source), size);
    const int closeResult = sceIoClose(fd);
    if (!written || closeResult < 0) {
        sceIoRemove(temporaryPath);
        lagi::platform::logging::writef(
            "[SavePlatform] write failed device=%d slot=%s size=%u\n",
            deviceId, fileName, static_cast<unsigned>(size));
        return -1;
    }

    sceIoRemove(backupPath);
    const bool hadPrevious = sceIoRename(path, backupPath) >= 0;
    if (sceIoRename(temporaryPath, path) < 0) {
        if (hadPrevious)
            sceIoRename(backupPath, path);
        sceIoRemove(temporaryPath);
        return -1;
    }
    if (hadPrevious)
        sceIoRemove(backupPath);

    lagi::platform::logging::writef(
        "[SavePlatform] wrote device=%d slot=%s size=%u\n",
        deviceId, fileName, static_cast<unsigned>(size));
    return 0;
}

extern "C" void lagiAzelSaveMarkInGameLoad()
{
    g_inGameLoadPending.store(true, std::memory_order_release);
}

extern "C" int lagiAzelSaveConsumeInGameLoad()
{
    return g_inGameLoadPending.exchange(
        false, std::memory_order_acq_rel) ? 1 : 0;
}

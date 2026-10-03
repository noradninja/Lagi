#include "lagi/lagi_azel_upstream_prelude.h"

#include "lagi/disc_image.h"
#include "lagi/platform.h"
#include "kernel/fileBundle.h"

#include <cstring>
#include <vector>

namespace {

bool readDiscFile(const char* fileName, std::vector<u8>& data)
{
    if (fileName && lagi::disc::read_file(fileName, data))
        return true;

    lagi::platform::logging::writef(
        "[AzelFS] missing disc file: %s\n",
        fileName ? fileName : "<null>");
    return false;
}

} // namespace

// Vita platform adapter for the upstream Azel file API. Azel continues to own
// all runtime and file-format behavior; only byte access is redirected from
// desktop stdio paths to Lagi's mounted ISO9660 disc image.

bool findFileOnDisc(const std::string& fileName)
{
    return lagi::disc::has_file(fileName.c_str());
}

u32 getFileSize(const char* fileName)
{
    std::uint32_t size = 0;
    return fileName && lagi::disc::file_size(fileName, size) ? size : 0;
}

int findMandatoryFileOnDisc(const char* fileName)
{
    return fileName && lagi::disc::has_file(fileName) ? 1 : 0;
}

u32 getFileSizeFromFileId(const char* fileName)
{
    return getFileSize(fileName);
}

int loadFile(const char* fileName, u8* destination, u16)
{
    if (!destination)
        return -1;

    std::vector<u8> data;
    if (!readDiscFile(fileName, data))
        return -1;

    std::memcpy(destination, data.data(), data.size());
    fileInfoStruct.m0 = 0;
    return 0;
}

int loadFile2(const char* fileName, u8* destination, u16 vdp1Pointer)
{
    return loadFile(fileName, destination, vdp1Pointer);
}

int loadFile(
    const char* fileName,
    s_fileBundle** destination,
    u16 vdp1Pointer)
{
    if (!destination)
        return -1;

    std::vector<u8> data;
    if (!readDiscFile(fileName, data))
        return -1;

    auto* raw = new u8[data.size()];
    std::memcpy(raw, data.data(), data.size());

    auto* bundle = new s_fileBundle(raw);
    if (vdp1Pointer)
        bundle->setPatchFilePointers(vdp1Pointer);

    *destination = bundle;
    fileInfoStruct.m0 = 0;
    return 0;
}

sSaturnMemoryFile::sSaturnMemoryFile(const char* fileName, u32 base)
    : m_name(fileName ? fileName : ""),
      m_base(base)
{
    std::vector<u8> data;
    if (!readDiscFile(fileName, data))
        return;

    m_data = new u8[data.size()];
    std::memcpy(m_data, data.data(), data.size());
    m_dataSize = static_cast<u32>(data.size());

    lagi::platform::logging::writef(
        "[AzelFS] loaded %s (%u bytes) from mounted disc\n",
        m_name.c_str(),
        static_cast<unsigned>(m_dataSize));
}

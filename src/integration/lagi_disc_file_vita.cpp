#include "lagi/lagi_azel_upstream_prelude.h"

#include "lagi/disc_image.h"
#include "lagi/platform.h"
#include "kernel/fileBundle.h"

#include <cstring>
#include <vector>
#include <unordered_map>

namespace {

// Engine-thread file service: completion means the synchronous destination
// copy finished, not merely that a filename exists on disc.
std::unordered_map<std::string,bool> completedLoads;
int completionDisc = -1;
std::string completionKey(const char* name)
{
    const int disc=lagi::disc::selected_disc();
    if(disc!=completionDisc){completedLoads.clear();completionDisc=disc;}
    std::string key=name?name:"";
    for(auto& c:key)if(c>='a' && c<='z')c=static_cast<char>(c-'a'+'A');
    return key;
}

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
    const auto key=completionKey(fileName);
    completedLoads[key]=false;
    if (!destination)
        return -1;

    std::vector<u8> data;
    if (!readDiscFile(fileName, data))
        return -1;

    std::memcpy(destination, data.data(), data.size());
    fileInfoStruct.m0 = 0;
    completedLoads[key]=true;
    return 0;
}

bool isSoundFileLoaded(sSaturnPtr filename)
{
    // Historical upstream name. Berserk scrolls pass SCB/PNB filename strings,
    // not sound-bank configuration records or a request to start a transfer.
    if(filename.isNull())return false;
    const auto key=completionKey(readSaturnString(filename).c_str());
    const auto found=completedLoads.find(key);
    return found!=completedLoads.end() && found->second;
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
    const auto key=completionKey(fileName);
    completedLoads[key]=false;
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
    completedLoads[key]=true;
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

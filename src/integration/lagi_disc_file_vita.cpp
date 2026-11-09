#include "lagi/lagi_azel_upstream_prelude.h"

#include "lagi/disc_image.h"
#include "lagi/prepared_disc_files.h"
#include "lagi/platform.h"
#include "kernel/fileBundle.h"

#include <cstring>
#include <vector>
#include <unordered_map>
#if LAGI_TRACK_RESIDENT_VIDEO
#include "lagi/resident_video.h"
#endif

namespace {

lagi::disc::PreparedFiles preparedFiles;

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
    if (preparedFiles.take(lagi::disc::selected_disc(), fileName, data))
        return true;
    if (fileName && lagi::disc::read_file(fileName, data))
        return true;

    lagi::platform::logging::writef(
        "[AzelFS] missing disc file: %s\n",
        fileName ? fileName : "<null>");
    return false;
}

} // namespace

namespace lagi::disc {
bool prepare_native_files(const std::vector<std::string>& names)
{
    return preparedFiles.prepare(selected_disc(), names,
        [](const char* name, std::vector<std::uint8_t>& bytes) {
            if (read_file(name, bytes)) return true;
            lagi::platform::logging::writef("[WorldMapPrepare] failed disc file: %s\n", name);
            return false;
        });
}
void clear_prepared_native_files() { preparedFiles.clear(); }
}

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
#if LAGI_TRACK_RESIDENT_VIDEO
    lagi::resident_video::record_load(fileName,destination,data.size());
#endif
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
      m_data(nullptr),
      m_dataSize(0),
      m_base(base)
{
    // Read-only checkpoints for camp save-load initialization. The crash
    // report places TWN_CAMP.PRG on the main-thread stack after the previous
    // TWN_RUIN.PRG load. Keep logging confined to those two overlays.
    const bool traceTownOverlay = m_name == "TWN_CAMP.PRG" ||
        m_name == "TWN_RUIN.PRG";
    if (traceTownOverlay)
        lagi::platform::logging::writef(
            "[TownOverlayLoadTrace] begin name=%s base=%08X\n",
            m_name.c_str(), static_cast<unsigned>(m_base));
    std::vector<u8> data;
    const bool readOk = readDiscFile(fileName, data);
    if (traceTownOverlay)
        lagi::platform::logging::writef(
            "[TownOverlayLoadTrace] read name=%s ok=%u bytes=%u\n",
            m_name.c_str(), readOk ? 1u : 0u,
            static_cast<unsigned>(data.size()));
    if (!readOk)
        return;

    m_data = new u8[data.size()];
    if (traceTownOverlay)
        lagi::platform::logging::writef(
            "[TownOverlayLoadTrace] allocated name=%s ptr=%p bytes=%u\n",
            m_name.c_str(), static_cast<void*>(m_data),
            static_cast<unsigned>(data.size()));
    std::memcpy(m_data, data.data(), data.size());
    m_dataSize = static_cast<u32>(data.size());
    if (traceTownOverlay)
        lagi::platform::logging::writef(
            "[TownOverlayLoadTrace] copied name=%s size=%u\n",
            m_name.c_str(), static_cast<unsigned>(m_dataSize));

    lagi::platform::logging::writef(
        "[AzelFS] loaded %s (%u bytes) from mounted disc\n",
        m_name.c_str(),
        static_cast<unsigned>(m_dataSize));
}

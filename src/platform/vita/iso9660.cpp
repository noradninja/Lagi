#include "lagi/disc_image.h"

#include <cstdio>
#include <cstring>
#include <cctype>
#include <dirent.h>
#include <string>
#include <vector>

namespace lagi::disc {

static constexpr std::uint32_t kLogicalSectorSize = 2048;
static std::string g_imagePath;
static std::uint32_t g_rootExtent = 0;
static std::uint32_t g_rootSize = 0;
static std::uint32_t g_physicalSectorSize = 2048;
static std::uint32_t g_userDataOffset = 0;
static std::uint32_t g_trackStartLba = 0;
static bool g_mounted = false;

static std::uint32_t le32(const std::uint8_t* p)
{
    return static_cast<std::uint32_t>(p[0]) |
           (static_cast<std::uint32_t>(p[1]) << 8) |
           (static_cast<std::uint32_t>(p[2]) << 16) |
           (static_cast<std::uint32_t>(p[3]) << 24);
}

static bool ends_with_ci(const char* name, const char* suffix)
{
    if (!name || !suffix) return false;
    const std::size_t n = std::strlen(name);
    const std::size_t s = std::strlen(suffix);
    if (n < s) return false;
    for (std::size_t i = 0; i < s; ++i) {
        if (std::tolower(static_cast<unsigned char>(name[n - s + i])) !=
            std::tolower(static_cast<unsigned char>(suffix[i])))
            return false;
    }
    return true;
}

static std::string trim(const std::string& in)
{
    std::size_t a = 0, b = in.size();
    while (a < b && std::isspace(static_cast<unsigned char>(in[a]))) ++a;
    while (b > a && std::isspace(static_cast<unsigned char>(in[b - 1]))) --b;
    return in.substr(a, b - a);
}

static bool parse_msf(const char* s, std::uint32_t& lba)
{
    unsigned m = 0, sec = 0, f = 0;
    if (std::sscanf(s, "%u:%u:%u", &m, &sec, &f) != 3) return false;
    lba = (m * 60u + sec) * 75u + f;
    return true;
}

static bool parse_cue()
{
    const char* dirPath = "ux0:data/lagi/Disc 1";
    DIR* dir = opendir(dirPath);
    if (!dir) return false;

    std::string cueName;
    while (dirent* entry = readdir(dir)) {
        if (ends_with_ci(entry->d_name, ".cue")) {
            cueName = entry->d_name;
            break;
        }
    }
    closedir(dir);
    if (cueName.empty()) return false;

    std::string cuePath = std::string(dirPath) + "/" + cueName;
    FILE* cue = std::fopen(cuePath.c_str(), "rb");
    if (!cue) return false;

    char line[512];
    std::string currentFile;
    bool inDataTrack = false;
    bool found = false;

    while (std::fgets(line, sizeof(line), cue)) {
        std::string s = trim(line);

        if (s.rfind("FILE ", 0) == 0) {
            const std::size_t q1 = s.find('"');
            const std::size_t q2 = q1 == std::string::npos ? std::string::npos : s.find('"', q1 + 1);
            if (q1 != std::string::npos && q2 != std::string::npos)
                currentFile = s.substr(q1 + 1, q2 - q1 - 1);
        } else if (s.rfind("TRACK ", 0) == 0) {
            inDataTrack = false;
            if (s.find("MODE1/2352") != std::string::npos) {
                g_physicalSectorSize = 2352;
                g_userDataOffset = 16;
                inDataTrack = true;
            } else if (s.find("MODE1/2048") != std::string::npos) {
                g_physicalSectorSize = 2048;
                g_userDataOffset = 0;
                inDataTrack = true;
            }
        } else if (inDataTrack && s.rfind("INDEX 01 ", 0) == 0) {
            std::uint32_t lba = 0;
            if (!parse_msf(s.c_str() + 9, lba))
                continue;
            g_trackStartLba = lba;
            if (!currentFile.empty()) {
                g_imagePath = std::string(dirPath) + "/" + currentFile;
                found = true;
                break;
            }
        }
    }

    std::fclose(cue);
    return found;
}

static bool read_logical_sector(FILE* f, std::uint32_t lba, void* dst)
{
    const std::uint64_t physicalLba = static_cast<std::uint64_t>(g_trackStartLba) + lba;
    const std::uint64_t offset = physicalLba * g_physicalSectorSize + g_userDataOffset;
    if (std::fseek(f, static_cast<long>(offset), SEEK_SET) != 0)
        return false;
    return std::fread(dst, 1, kLogicalSectorSize, f) == kLogicalSectorSize;
}

static bool read_extent(FILE* f, std::uint32_t extent, void* dst, std::size_t size)
{
    std::uint8_t* out = static_cast<std::uint8_t*>(dst);
    std::uint8_t sector[kLogicalSectorSize];
    std::size_t done = 0;
    std::uint32_t lba = extent;

    while (done < size) {
        if (!read_logical_sector(f, lba++, sector))
            return false;
        const std::size_t chunk = (size - done > kLogicalSectorSize)
            ? kLogicalSectorSize : size - done;
        std::memcpy(out + done, sector, chunk);
        done += chunk;
    }
    return true;
}

static bool name_matches(const std::uint8_t* raw, std::uint8_t len, const char* wanted)
{
    std::size_t actualLen = len;
    for (std::size_t i = 0; i < actualLen; ++i) {
        if (raw[i] == ';') { actualLen = i; break; }
    }

    const std::size_t wantedLen = std::strlen(wanted);
    if (actualLen != wantedLen) return false;

    for (std::size_t i = 0; i < actualLen; ++i) {
        if (std::toupper(static_cast<unsigned char>(raw[i])) !=
            std::toupper(static_cast<unsigned char>(wanted[i])))
            return false;
    }
    return true;
}

struct Entry {
    std::uint32_t extent = 0;
    std::uint32_t size = 0;
    bool valid = false;
};

static Entry find_root_file(FILE* f, const char* wanted)
{
    Entry result{};
    std::vector<std::uint8_t> root(g_rootSize);
    if (!read_extent(f, g_rootExtent, root.data(), root.size()))
        return result;

    std::size_t pos = 0;
    while (pos < root.size()) {
        const std::uint8_t recordLen = root[pos];
        if (recordLen == 0) {
            pos = ((pos / kLogicalSectorSize) + 1) * kLogicalSectorSize;
            continue;
        }
        if (pos + recordLen > root.size() || recordLen < 34)
            break;

        const std::uint8_t* rec = root.data() + pos;
        const std::uint8_t nameLen = rec[32];
        if (33u + nameLen <= recordLen && name_matches(rec + 33, nameLen, wanted)) {
            result.extent = le32(rec + 2);
            result.size = le32(rec + 10);
            result.valid = true;
            return result;
        }
        pos += recordLen;
    }
    return result;
}

bool init()
{
    g_mounted = false;
    g_imagePath.clear();
    g_rootExtent = 0;
    g_rootSize = 0;
    g_physicalSectorSize = 2048;
    g_userDataOffset = 0;
    g_trackStartLba = 0;

    if (!parse_cue())
        return false;

    FILE* f = std::fopen(g_imagePath.c_str(), "rb");
    if (!f)
        return false;

    std::uint8_t pvd[kLogicalSectorSize]{};
    const bool ok = read_logical_sector(f, 16, pvd);
    if (!ok || pvd[0] != 1 || std::memcmp(pvd + 1, "CD001", 5) != 0) {
        std::fclose(f);
        return false;
    }

    const std::uint8_t* root = pvd + 156;
    if (root[0] < 34) {
        std::fclose(f);
        return false;
    }

    g_rootExtent = le32(root + 2);
    g_rootSize = le32(root + 10);
    std::fclose(f);

    g_mounted = g_rootExtent != 0 && g_rootSize != 0;
    return g_mounted;
}

bool mounted() { return g_mounted; }
const char* image_path() { return g_imagePath.c_str(); }

bool has_file(const char* name)
{
    if (!g_mounted) return false;
    FILE* f = std::fopen(g_imagePath.c_str(), "rb");
    if (!f) return false;
    const Entry e = find_root_file(f, name);
    std::fclose(f);
    return e.valid;
}

bool read_file(const char* name, std::vector<std::uint8_t>& out)
{
    out.clear();
    if (!g_mounted) return false;

    FILE* f = std::fopen(g_imagePath.c_str(), "rb");
    if (!f) return false;

    const Entry e = find_root_file(f, name);
    if (!e.valid) {
        std::fclose(f);
        return false;
    }

    out.resize(e.size);
    const bool ok = read_extent(f, e.extent, out.data(), out.size());
    std::fclose(f);

    if (!ok) out.clear();
    return ok;
}

} // namespace lagi::disc

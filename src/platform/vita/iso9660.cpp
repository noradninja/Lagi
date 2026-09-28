#include "lagi/disc_image.h"

#include <cstdio>
#include <cstring>
#include <cctype>
#include <dirent.h>
#include <string>

namespace lagi::disc {

static constexpr std::uint32_t kSectorSize = 2048;
static std::string g_imagePath;
static std::uint32_t g_rootExtent = 0;
static std::uint32_t g_rootSize = 0;
static bool g_mounted = false;

static std::uint32_t le32(const std::uint8_t* p)
{
    return static_cast<std::uint32_t>(p[0]) |
           (static_cast<std::uint32_t>(p[1]) << 8) |
           (static_cast<std::uint32_t>(p[2]) << 16) |
           (static_cast<std::uint32_t>(p[3]) << 24);
}

static bool ends_with_iso(const char* name)
{
    if (!name) return false;
    const std::size_t n = std::strlen(name);
    if (n < 4) return false;
    return name[n-4] == '.' &&
           std::tolower(static_cast<unsigned char>(name[n-3])) == 'i' &&
           std::tolower(static_cast<unsigned char>(name[n-2])) == 's' &&
           std::tolower(static_cast<unsigned char>(name[n-1])) == 'o';
}

static bool find_image()
{
    DIR* dir = opendir("ux0:data/lagi");
    if (!dir) return false;

    bool found = false;
    while (dirent* entry = readdir(dir)) {
        if (ends_with_iso(entry->d_name)) {
            g_imagePath = "ux0:data/lagi/";
            g_imagePath += entry->d_name;
            found = true;
            break;
        }
    }
    closedir(dir);
    return found;
}

static bool read_at(FILE* f, std::uint64_t offset, void* dst, std::size_t size)
{
    if (std::fseek(f, static_cast<long>(offset), SEEK_SET) != 0)
        return false;
    return std::fread(dst, 1, size, f) == size;
}

static bool name_matches(const std::uint8_t* raw, std::uint8_t len, const char* wanted)
{
    std::size_t actualLen = len;
    for (std::size_t i = 0; i < actualLen; ++i) {
        if (raw[i] == ';') {
            actualLen = i;
            break;
        }
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
    if (!read_at(f, static_cast<std::uint64_t>(g_rootExtent) * kSectorSize,
                 root.data(), root.size()))
        return result;

    std::size_t pos = 0;
    while (pos < root.size()) {
        const std::uint8_t recordLen = root[pos];
        if (recordLen == 0) {
            pos = ((pos / kSectorSize) + 1) * kSectorSize;
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

    if (!find_image())
        return false;

    FILE* f = std::fopen(g_imagePath.c_str(), "rb");
    if (!f)
        return false;

    std::uint8_t pvd[kSectorSize]{};
    const bool ok = read_at(f, 16ull * kSectorSize, pvd, sizeof(pvd));
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
    const bool ok = read_at(f, static_cast<std::uint64_t>(e.extent) * kSectorSize,
                            out.data(), out.size());
    std::fclose(f);
    if (!ok) out.clear();
    return ok;
}

} // namespace lagi::disc

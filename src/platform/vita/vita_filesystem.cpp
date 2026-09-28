#include "lagi/platform.h"
#include "lagi/disc_image.h"
#include <cstdio>

namespace lagi::platform::filesystem {
static constexpr const char* root = "ux0:data/lagi";

bool init() { return true; }
const char* data_root() { return root; }

bool game_data_present()
{
    char p[256];
    std::snprintf(p, sizeof(p), "%s/COMMON.DAT", root);
    FILE* f = std::fopen(p, "rb");
    if (f) {
        std::fclose(f);
        return true;
    }

    if (!lagi::disc::mounted())
        lagi::disc::init();
    return lagi::disc::mounted() && lagi::disc::has_file("COMMON.DAT");
}
}

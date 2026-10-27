#include "lagi/lagi_transient_laser_lights.h"
#include <cassert>
int main() {
    using namespace lagi;
    static_assert(kMaxTransientLaserLights == 4);
    static_assert(quarter_rgb888(0xF8F880u) == 0x3E3E20u);
    static_assert(quarter_rgb888(0xFFFFFFu) == 0x3F3F3Fu);
    static_assert(gouraud555_hue_at_peak(0x801Fu, 248u) == 0xF80000u);
    static_assert(gouraud555_hue_at_peak(0xE280u, 31u) == 0x00191Fu);
    static_assert(saturate_rgb888_peak(0x171406u) == 0x171200u);
    static_assert(saturate_rgb888_peak(0x141414u) == 0x141414u);
    TransientLaserLightFrame frame;
    for (unsigned i = 1; i <= 4; ++i) {
        TransientLaserLight light{};
        light.source = i;
        light.worldPosition[0] = static_cast<std::int32_t>(i * 0x10000);
        light.color[2] = static_cast<std::uint16_t>(i * 4);
        light.range = 0x14000;
        assert(frame.upsert(light));
    }
    assert(frame.count == 4);
    TransientLaserLight fifth{};
    fifth.source = 5;
    fifth.range = 0x14000;
    assert(!frame.upsert(fifth));
    assert(frame.count == 4);
    TransientLaserLight updated{};
    updated.source = 2;
    updated.worldPosition[0] = 0x450000;
    updated.color[2] = 31;
    updated.range = 0x14000;
    assert(frame.upsert(updated));
    assert(frame.count == 4);
    assert(frame.lights[1].worldPosition[0] == 0x450000);
    assert(frame.lights[1].color[2] == 31);
    assert(frame.lights[3].source == 4);
    frame.clear();
    assert(frame.count == 0);
    assert(!frame.upsert({}));
}

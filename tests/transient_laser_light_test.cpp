#include "lagi/lagi_transient_laser_lights.h"
#include <cassert>
int main() {
    using namespace lagi;
    static_assert(kMaxTransientLaserLights == 4);
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

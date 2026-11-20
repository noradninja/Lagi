#include <cassert>
#include <cmath>
#include "../include/lagi/native_scatter_flight.h"

int main()
{
    using lagi::nativeScatterFlightOffset;
    for (int dx : {-0x30000, 0, 0x30000})
        for (int dz : {-0x40000, 0, 0x40000})
        {
            const auto v = nativeScatterFlightOffset(dx, 0x20000, dz);
            const double length = std::sqrt(double(v[0])*v[0] +
                                            double(v[1])*v[1] +
                                            double(v[2])*v[2]);
            assert(length > 0x5FF00 && length < 0x60100);
            if (dx != 0) assert((v[0] > 0) == (dx > 0));
            if (dz != 0) assert((v[2] > 0) == (dz > 0));
        }
    const auto center = nativeScatterFlightOffset(0, 0, 0);
    assert(center[1] == -0x60000);
}

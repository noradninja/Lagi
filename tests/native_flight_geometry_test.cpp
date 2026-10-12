#include "lagi/lagi_radar_geometry.h"
#include "lagi/lagi_screen_depth.h"
#include "lagi/lagi_vdp1_quad_geometry.h"
#include <cassert>
#include <cmath>
#include <cstdio>

int main()
{
    unsigned cases = 0;
    for (int x : {-29, 0, 125, 205}) for (int y : {-33, 0, 63, 96})
    for (unsigned f = 0; f < 16; ++f) {
        const auto q = lagi::native_radar_marker_quad(x, y, f);
        const unsigned type = f & 3;
        if (type == 0 || type == 3) {
            assert(q[0][0] == x && q[0][1] == y);
            assert(q[2][0] == x+1 && q[2][1] == y-1);
        } else {
            const int r = type == 1 ? 1 : 2;
            assert(q[0][1] == y+r && q[1][1] == y+r);
            assert(q[2][1] == y-r && q[3][1] == y-r);
            assert(q[0][0] == ((f & 4) ? x : x-r));
            assert(q[1][0] == ((f & 4) ? x : x+r));
            assert(q[2][0] == ((!(f & 4) && (f & 8)) ? x : x+r));
            assert(q[3][0] == ((!(f & 4) && (f & 8)) ? x : x-r));
        }
        ++cases;
    }
    const float nearZ = 0.1875f, farZ = 32.0f;
    assert(std::abs(lagi::native_screen_depth(nearZ, nearZ, farZ)) < 1e-6f);
    assert(std::abs(lagi::native_screen_depth(farZ, nearZ, farZ)-1.0f) < 1e-6f);
    float last = -1;
    for (unsigned i = 0; i <= 4096; ++i) {
        const float z = nearZ + (farZ-nearZ) * i/4096.0f;
        const float actual = lagi::native_screen_depth(z, nearZ, farZ);
        const float matrixDepth = ((farZ/(farZ-nearZ))*z-nearZ*farZ/(farZ-nearZ))/z;
        assert(std::abs(actual-matrixDepth) < 2e-6f && actual >= last);
        last = actual;
        ++cases;
    }
    assert(lagi::native_screen_depth(nearZ/2, nearZ, farZ) < 0);
    assert(lagi::native_screen_depth(farZ*2, nearZ, farZ) > 1);
    for (const auto delta : {std::array<float,2>{0,0}, {30,0}, {0,30}, {30,30}, {-30,30}})
    for (bool across : {false,true}) {
        const std::array<float,2> a={4,7},b={4+delta[0],7+delta[1]};
        lagi::NativeScreenQuad q=across?lagi::NativeScreenQuad{a,b,b,a}:lagi::NativeScreenQuad{a,a,b,b};
        assert(lagi::expand_native_collapsed_quad(q));
        float area=0;
        for(unsigned i=0;i<4;++i){const auto j=(i+1)&3;area+=q[i][0]*q[j][1]-q[j][0]*q[i][1];}
        const float expected=std::sqrt(delta[0]*delta[0]+delta[1]*delta[1])+1;
        assert(std::abs(std::abs(area)*0.5f-expected)<0.001f);
        ++cases;
    }
    lagi::NativeScreenQuad ordinary={{{0,0},{8,0},{8,8},{0,8}}};
    const auto original=ordinary;
    assert(!lagi::expand_native_collapsed_quad(ordinary) && ordinary==original);
    lagi::NativeScreenQuad triangle={{{0,0},{8,0},{8,8},{0,0}}};
    const auto originalTriangle=triangle;
    assert(!lagi::expand_native_collapsed_quad(triangle) && triangle==originalTriangle);
    std::printf("PASS: %u native marker shape/perspective depth cases; not GPU or Vita validation\n", cases);
}

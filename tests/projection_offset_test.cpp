#include "lagi/lagi_projection_offset.h"
#include "lagi/lagi_render_bridge.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>

#ifdef LAGI_TEST_CAPTURE
struct { struct { std::int16_t m44_localCoordinatesX, m46_localCoordinatesY; } m405C; }
    graphicEngineStatus;
namespace lagi::scene_bridge {
#include "projection_capture_production.inc"
}
namespace lagi::azel_bridge {
struct TestMatrix {std::int32_t raw[12];};
TestMatrix* pCurrentMatrix=nullptr;
bool g_viewRelativeScope=false,g_nativeSceneViewValid=false;
std::int32_t g_viewScopeMatrix[12]{},g_nativeSceneViewMatrix[12]{};
void copyMatrixRaw(const TestMatrix& m,std::int32_t out[12]){std::memcpy(out,m.raw,sizeof(m.raw));}
#include "projection_scope_production.inc"
}
#endif

static bool close(float a, float b) { return std::fabs(a-b) < 0.0001f; }

int main()
{
    lagi::azel_bridge::SubmissionState defaults;
    assert(defaults.localCoordinatesX == 176 && defaults.localCoordinatesY == 112);
#ifdef LAGI_TEST_CAPTURE
    using namespace lagi::azel_bridge;
    TestMatrix root{{65536,0,0,0,0,65536,0,0,0,0,65536,0}};
    pCurrentMatrix=&root;g_nativeSceneViewValid=false;
    begin_view_relative_submission_scope(true);
    assert(g_nativeSceneViewValid && g_viewRelativeScope && g_nativeSceneViewMatrix[0]==65536);
    end_view_relative_submission_scope();assert(!g_viewRelativeScope);
    root.raw[3]=77;begin_view_relative_submission_scope(false);
    assert(g_nativeSceneViewMatrix[3]==0 && g_viewScopeMatrix[3]==77);
    begin_view_relative_submission_scope(true);assert(g_nativeSceneViewMatrix[3]==77);
    pCurrentMatrix=nullptr;begin_view_relative_submission_scope(true);assert(!g_viewRelativeScope);
    graphicEngineStatus.m405C = {120,112};
    lagi::scene_bridge::capture_local_coordinates(defaults.localCoordinatesX, defaults.localCoordinatesY);
    graphicEngineStatus.m405C = {176,112}; // Native cleanup after submission.
    assert(defaults.localCoordinatesX == 120 && defaults.localCoordinatesY == 112);
    std::int16_t x,y;
    lagi::scene_bridge::capture_local_coordinates(x,y);
    assert(x == 176 && y == 112);
#endif
    const float views[][12] = {
        {1,0,0,0, 0,1,0,0, 0,0,1,0},
        {0,0,1,2, 0,1,0,-3, -1,0,0,7},
        {0.8f,0,0.6f,2, 0,1,0,-3, -0.6f,0,0.8f,7},
        // Invert actual basis, not its transpose (fixed-point rounding).
        {0.799f,0,0.601f,2, 0,0.999f,0,-3, -0.6f,0,0.8f,7}
    };
    unsigned cases = 0;
    for (auto& view : views) for (float aspect : {960.f/544, 640.f/384, 4.f/3})
    for (int x : {120,176,220}) for (int y : {90,112,140})
    for (float depth : {0.5f,2.f,20.f}) {
        const float canvas=(4.f/3)/aspect, px=1.1f*canvas, py=1.4f;
        float delta[3];
        const bool shifted=lagi::nativeProjectionOffset(x,y,canvas,px,py,view,delta);
        assert(shifted == (x!=176 || y!=112));
        float dv[3]{};
        for(int row=0;row<3;++row)
            for(int axis=0;axis<3;++axis)
                dv[row]+=view[row*4+axis]*delta[axis]*depth;
        assert(close(dv[0]*px/depth, (x-176)/176.f*canvas));
        assert(close(dv[1]*py/depth, -(y-112)/112.f));
        assert(close(dv[2],0)); // No change to occlusion / near/far depth.
        ++cases;
    }
    float delta[3], singular[12]{};
    assert(!lagi::nativeProjectionOffset(120,112,1,1,1,singular,delta));
    assert(!lagi::nativeProjectionOffset(120,112,1,0,1,views[0],delta));
    std::printf("PASS: %u native per-submission projection cases\n",cases);
}

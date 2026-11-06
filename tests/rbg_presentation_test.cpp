#include "../include/lagi/saturn_presentation.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <initializer_list>
using std::floor;
// Actual production shader helper, extracted by the runner.
#include "rbg_projection_shader.inc"

int main() {
    unsigned checks=0;
    for(const unsigned width : {480u,960u}) {
        const unsigned height=width==480?272:544;
        const float scale=lagi::saturnDisplayXScale(width,height);
        const float inverse=lagi::rbgProjectionInverseXScale(false,width,height);
        assert(std::abs(scale*inverse-1.f)<1e-6f);
        assert(lagi::rbgProjectionInverseXScale(true,width,height)==1.f);
        // Frontend retains all 352 original pixels, including both boundaries.
        for(int x=0;x<352;++x) assert(rbgProjectionX(float(x),1.f)==float(x));
        // Native ground inverse projection at zero pitch/yaw. For a fixed
        // world landmark, changing camera X/Z/height must recover that same
        // landmark after the existing 3D wide-view projection. The old RBG
        // fullscreen mapping instead recovers a camera-dependent X.
        bool oldMappingDrifts=false;
        for(float cameraX : {-3.f,-1.f,0.f,1.f,3.f})
        for(float cameraZ : {-2.f,0.f,2.f})
        for(float height : {0.5f,1.f,2.f})
        for(float worldX : {-2.f,0.f,2.f}) {
            const float worldZ=8.f;
            const float depth=worldZ-cameraZ;
            const float nativeX=176.f-200.f*(worldX-cameraX)/depth;
            const float nativeY=112.f+160.f*height/depth;
            const float displayX=176.f+(nativeX-176.f)*scale;
            const float correctedX=rbgProjectionX(displayX,inverse);
            const float recoveredX=cameraX-height*0.8f*(correctedX-176.f)/(nativeY-112.f);
            const float oldX=cameraX-height*0.8f*(displayX-176.f)/(nativeY-112.f);
            // Point sampling permits one native pixel of quantization.
            assert(std::abs(recoveredX-worldX)<=depth/200.f+1e-5f);
            oldMappingDrifts|=std::abs(oldX-worldX)>0.1f;
            ++checks;
        }
        assert(oldMappingDrifts);
        // Wide view evaluates additional native rays rather than clamping
        // to the old raster's edge (which would smear/pin the plane).
        assert(rbgProjectionX(0.f,inverse)<0.f);
        assert(rbgProjectionX(351.f,inverse)>351.f);
    }
    assert(lagi::saturnDisplayXScale(640,480)==1.f);
    std::printf("PASS: %u translated-camera ground alignments; frontend identity; extended wide rays\n",checks);
}

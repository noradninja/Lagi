#include "lagi/lagi_render_bridge.h"
#include "lagi/lagi_point_light.h"
#include <algorithm>
#include <cstring>
#include <cstdio>
namespace lagi::platform::logging { void writef(const char*,...) {} }
using u16=std::uint16_t;
using s32=std::int32_t;
using SubmissionState=lagi::azel_bridge::SubmissionState;
struct LagiMatrix4x3 { s32 raw[12]; } matrix{{65536,0,0,0,0,65536,0,0,0,0,65536,0}};
LagiMatrix4x3* pCurrentMatrix=&matrix;
u16 loc_601FA9E=0x6103;
s32 pointLightParams[5]={65536,0,0,0x14000000,0x19000};
void copyMatrixRaw(const LagiMatrix4x3& in,s32 out[12]) { std::memcpy(out,in.raw,sizeof(in.raw)); }
struct {u16 color[3]={6,20,23};} currentLightVector_M;
#include "point_state_production.inc"
#include "point_capture_production.inc"
#include "point_consumer_production.inc"
int main()
{
    SubmissionState state;
    std::copy(matrix.raw,matrix.raw+12,state.modelMatrix);
    state.lightVector[0]=100;
    capture_native_point_light(state);
    if (state.hasNativeModelLight) return 1;
    loc_601FA9E=0xA11D;
    capture_native_point_light(state);
    if (!state.hasNativeModelLight || state.nativeModelLight[0]!=4096) return 2;
    // Consumer must not apply even a deliberately different world matrix.
    state.modelMatrix[0]=2*65536;
    s32 consumed[3]; transformLiveLightVectorToModelSpace(state,consumed);
    if (consumed[0]!=4096 || consumed[1] || consumed[2]) return 3;
    loc_601FA9E=0x6103;
    capture_native_point_light(state);
    transformLiveLightVectorToModelSpace(state,consumed);
    if (state.hasNativeModelLight || consumed[0]!=200) return 4;
    loc_601FA9E=0xA11D;
    matrix.raw[3]=65536;
    capture_native_point_light(state);
    if (!state.hasNativeModelLight || state.nativeModelLight[0]) return 5;
    pCurrentMatrix=nullptr;
    capture_native_point_light(state);
    if (state.hasNativeModelLight) return 6;
    // A direction-only camera transform leaves the world position far away,
    // reproducing the hardware outside-range failure. A translated position
    // lives beside the view/model origin and produces a nonzero local vector.
    s32 viewModel[12]={65536,0,0,0,0,65536,0,0,0,0,65536,0};
    s32 wrongPosition[5]={80*65536,0xA000,0,0x14000000,0x19000};
    s32 translatedPosition[5]={0,0xA000,0,0x14000000,0x19000};
    auto wrong=lagi::native_point_light(viewModel,wrongPosition);
    auto corrected=lagi::native_point_light(viewModel,translatedPosition);
    if (!wrong.outside || corrected.outside || !corrected.vector[1]) return 7;
    puts("PASS: actual point capture/renderer consumer, native versus world matrix, activation/reset and null matrix (host; not GPU rendering)");
}

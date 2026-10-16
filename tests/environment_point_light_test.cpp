#include "lagi/lagi_render_bridge.h"
#include "lagi/lagi_point_light.h"
#include <algorithm>
#include <cassert>
#include <cstring>
#include <cstdio>
namespace lagi::platform::logging {void writef(const char*,...) {}}
using SubmissionState=lagi::azel_bridge::SubmissionState;
using RenderSubmission=lagi::azel_bridge::RenderSubmission;
using s32=std::int32_t;using u16=std::uint16_t;
struct LagiMatrix4x3 {s32 raw[12];} matrix{{65536,0,0,0,0,65536,0,0,0,0,65536,0}};
LagiMatrix4x3* pCurrentMatrix=&matrix;
u16 loc_601FA9E=0x6103;
s32 pointLightParams[5]={65536,0,0,0x14000000,0x19000};
struct {u16 color[3]={6,20,23};} currentLightVector_M;
struct {std::uint32_t falloff[3]={0x010203,0x040506,0x070809};} lightSetup;
void copyMatrixRaw(const LagiMatrix4x3& in,s32 out[12]){std::memcpy(out,in.raw,sizeof(in.raw));}
bool g_forceDynamicSubmissions,g_nativeSceneViewValid,g_viewRelativeScope,g_hasPendingTownSubmission;
unsigned g_submissionCount,g_modelCacheMisses,g_explicitStaticContextsSet,g_explicitStaticContextsConsumed;
void* g_lastModel;void* g_lastAdaptedModel;
SubmissionState g_lastState;
std::vector<int> g_adaptedModels,g_submissions,g_vdp1UiCommands;
RenderSubmission g_pendingTownSubmission;
#include "environment_state.inc"
#include "environment_begin.inc"
#include "environment_capture.inc"
#include "environment_polygon_type.inc"
#include "environment_consumer.inc"
LivePolygonLightState cachedPayload(const SubmissionState& state){
 RenderSubmission submission;submission.state=state;
 #include "environment_cached_payload.inc"
 return light;
}
SubmissionState baseline(){SubmissionState s;s.hasLight=true;s.fieldEnvironment=true;
 s.lightVector[0]=123;s.lightColor[0]=99;s.lightFalloff[0]=0xABCDEF;return s;}
int main(){
 begin_frame(false);assert(!g_fieldEnvironmentPointLight.active);
 auto state=baseline();apply_field_environment_point_light(state);assert(!state.hasNativeModelLight && state.lightColor[0]==99);
 capture_field_environment_point_light();assert(!g_fieldEnvironmentPointLight.active);
 loc_601FA9E=0xA11D;capture_field_environment_point_light();assert(g_fieldEnvironmentPointLight.active);
 // Native cleanup restores the selector and globals; the extension must use
 // its own captured position, color and falloff, never hold the selector open.
 loc_601FA9E=0x6103;pointLightParams[0]=100*65536;currentLightVector_M.color[0]=80;lightSetup.falloff[0]=0;
 apply_field_environment_point_light(state);
 assert(state.hasNativeModelLight && state.hasLight && state.nativeModelLight[0]==4096);
 assert(state.lightColor[0]==6 && state.lightColor[1]==20 && state.lightColor[2]==23 && state.lightFalloff[0]==0x010203);
 assert(!state.dynamic && loc_601FA9E==0x6103);
 auto cached=cachedPayload(state);
 assert(cached.valid && cached.vector[0]==4096 && cached.color[0]==6 && cached.falloff[0]==0x010203);
 auto normal=baseline();normal.fieldEnvironment=false;apply_field_environment_point_light(normal);assert(!normal.hasNativeModelLight);
 normal=baseline();normal.billboard=true;apply_field_environment_point_light(normal);assert(!normal.hasNativeModelLight);
 normal=baseline();normal.hasNativeModelLight=true;normal.nativeModelLight[0]=17;
 apply_field_environment_point_light(normal);assert(normal.nativeModelLight[0]==17 && normal.lightColor[0]==99);
 matrix.raw[3]=100*65536;normal=baseline();apply_field_environment_point_light(normal);
 assert(!normal.hasNativeModelLight && normal.lightVector[0]==123 && normal.lightColor[0]==99 && normal.lightFalloff[0]==0xABCDEF);
 matrix.raw[3]=0;pCurrentMatrix=nullptr;normal=baseline();apply_field_environment_point_light(normal);assert(!normal.hasNativeModelLight);pCurrentMatrix=&matrix;
 // Exact native view/model matrix is used, not a world-space state matrix.
 normal=baseline();normal.modelMatrix[0]=2*65536;apply_field_environment_point_light(normal);assert(normal.nativeModelLight[0]==4096);
 matrix.raw[0]=0;matrix.raw[2]=65536;matrix.raw[8]=-65536;matrix.raw[10]=0;
 normal=baseline();apply_field_environment_point_light(normal);assert(normal.nativeModelLight[0]==0 && normal.nativeModelLight[2]==4096);
 // A new frame with no special light must restore cached-mesh light state.
 begin_frame(false);normal=baseline();apply_field_environment_point_light(normal);
 assert(!g_fieldEnvironmentPointLight.active && !normal.hasNativeModelLight && normal.lightColor[0]==99);
 normal.modelMatrix[0]=65536;cached=cachedPayload(normal);
 assert(cached.valid && cached.vector[0]==123 && cached.color[0]==99 && cached.falloff[0]==0xABCDEF);
 loc_601FA9E=0xA11D;capture_field_environment_point_light();assert(g_fieldEnvironmentPointLight.active);
 loc_601FA9E=0x6103;capture_field_environment_point_light();assert(!g_fieldEnvironmentPointLight.active);
 puts("PASS: production environment snapshot/application/frame reset, copied color/range, native matrix, outside-range preservation and actor/UI exclusion; GPU/device untested");
}

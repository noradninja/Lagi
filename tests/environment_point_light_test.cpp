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
struct RawFixed {s32 raw=700;s32 asS32()const{return raw;}};
struct {RawFixed lightVector[3];u16 color[3]={6,20,23};} currentLightVector_M;
struct {std::uint32_t falloff[3]={0x010203,0x040506,0x070809};} lightSetup;
void copyMatrixRaw(const LagiMatrix4x3& in,s32 out[12]){std::memcpy(out,in.raw,sizeof(in.raw));}
bool g_forceDynamicSubmissions,g_nativeSceneViewValid,g_viewRelativeScope,g_hasPendingTownSubmission;
s32 g_nativeSceneViewMatrix[12]={65536,0,0,0,0,65536,0,0,0,0,65536,0};
unsigned g_submissionCount,g_modelCacheMisses,g_explicitStaticContextsSet,g_explicitStaticContextsConsumed;
void* g_lastModel;void* g_lastAdaptedModel;
SubmissionState g_lastState;
std::vector<int> g_adaptedModels,g_submissions,g_vdp1UiCommands;
RenderSubmission g_pendingTownSubmission;
#include "environment_state.inc"
#include "environment_begin.inc"
#include "actor_point_capture.inc"
#include "environment_capture.inc"
#include "actor_current_capture.inc"
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
 assert(!state.hasNativeModelLight && state.hasLight && state.additionalLight.active && state.additionalLight.vector[0]==4096);
 assert(state.lightColor[0]==99 && state.lightFalloff[0]==0xABCDEF);
 assert(state.additionalLight.color[0]==6 && state.additionalLight.color[1]==20 && state.additionalLight.color[2]==23);
 assert(!state.dynamic && loc_601FA9E==0x6103);
 auto cached=cachedPayload(state);
 assert(cached.valid && cached.color[0]==99 && cached.falloff[0]==0xABCDEF && cached.additionalLight.vector[0]==4096);
 std::int16_t facing[3]={16384,0,0};int combined[3]={1000,1100,1200};
 lagi::accumulate_additional_model_light(cached.additionalLight,facing,combined);
 assert(combined[0]==1000+23*1024 && combined[1]==1100+20*1024 && combined[2]==1200+6*1024);
 std::int16_t away[3]={-16384,0,0};int unchanged[3]={1000,1100,1200};
 lagi::accumulate_additional_model_light(cached.additionalLight,away,unchanged);
 assert(unchanged[0]==1000 && unchanged[1]==1100 && unchanged[2]==1200);
 auto normal=baseline();normal.fieldEnvironment=false;apply_field_environment_point_light(normal);assert(!normal.additionalLight.active);
 normal=baseline();normal.billboard=true;apply_field_environment_point_light(normal);assert(!normal.additionalLight.active);
 normal=baseline();normal.hasNativeModelLight=true;normal.nativeModelLight[0]=17;
 apply_field_environment_point_light(normal);assert(normal.nativeModelLight[0]==17 && normal.lightColor[0]==99 && !normal.additionalLight.active);
 matrix.raw[3]=100*65536;normal=baseline();apply_field_environment_point_light(normal);
 assert(!normal.additionalLight.active && normal.lightVector[0]==123 && normal.lightColor[0]==99 && normal.lightFalloff[0]==0xABCDEF);
 matrix.raw[3]=0;pCurrentMatrix=nullptr;normal=baseline();apply_field_environment_point_light(normal);assert(!normal.additionalLight.active);pCurrentMatrix=&matrix;
 // Exact native view/model matrix is used, not a world-space state matrix.
 normal=baseline();normal.modelMatrix[0]=2*65536;apply_field_environment_point_light(normal);assert(normal.additionalLight.vector[0]==4096);
 matrix.raw[0]=0;matrix.raw[2]=65536;matrix.raw[8]=-65536;matrix.raw[10]=0;
 normal=baseline();apply_field_environment_point_light(normal);assert(normal.additionalLight.vector[0]==0 && normal.additionalLight.vector[2]==4096);
 // A new frame with no special light must restore cached-mesh light state.
 begin_frame(false);normal=baseline();apply_field_environment_point_light(normal);
 assert(!g_fieldEnvironmentPointLight.active && !normal.additionalLight.active && normal.lightColor[0]==99);
 normal.modelMatrix[0]=65536;cached=cachedPayload(normal);
 assert(cached.valid && cached.vector[0]==123 && cached.color[0]==99 && cached.falloff[0]==0xABCDEF);
 int maxed[3]={2147483640,2147483640,2147483640};
 lagi::AdditionalModelLight extreme{{32767,32767,32767},{65535,65535,65535},true};
 std::int16_t extremeNormal[3]={32767,32767,32767};
 lagi::accumulate_additional_model_light(extreme,extremeNormal,maxed);
 assert(maxed[0]==2147483647 && maxed[1]==2147483647 && maxed[2]==2147483647);
 loc_601FA9E=0xA11D;capture_field_environment_point_light();assert(g_fieldEnvironmentPointLight.active);
 loc_601FA9E=0x6103;capture_field_environment_point_light();assert(!g_fieldEnvironmentPointLight.active);
 // Actor path: preserve the authored normal-light RGB/direction while the
 // live native globals hold special point parameters and their own color.
 begin_frame(false);matrix={{65536,0,0,0,0,65536,0,0,0,0,65536,0}};
 pointLightParams[0]=65536;currentLightVector_M.color[0]=6;
 capture_field_actor_base_light(-65536,0,0,0x112233);
 assert(g_fieldActorBaseLight.active);
 loc_601FA9E=0xA11D;SubmissionState actor;actor.dynamic=true;
 std::copy(matrix.raw,matrix.raw+12,actor.modelMatrix);::capture_current_light(actor);
 assert(actor.hasLight && !actor.hasNativeModelLight && actor.additionalLight.active);
 assert(actor.lightVector[0]==4096 && actor.lightColor[0]==0x11 && actor.lightColor[1]==0x22 && actor.lightColor[2]==0x33);
 assert(actor.additionalLight.vector[0]==4096 && actor.additionalLight.color[0]==6 && actor.additionalLight.color[2]==23);
 cached=cachedPayload(actor);assert(cached.vector[0]==4096 && cached.color[2]==0x33 && cached.additionalLight.active);
 int actorCombined[3]={0x33*1024,0x22*1024,0x11*1024};
 lagi::accumulate_additional_model_light(cached.additionalLight,facing,actorCombined);
 assert(actorCombined[0]==(0x33+23)*1024 && actorCombined[1]==(0x22+20)*1024 && actorCombined[2]==(0x11+6)*1024);
 g_nativeSceneViewValid=true;
 const s32 rotatedView[12]={0,0,65536,0,0,65536,0,0,-65536,0,0,0};
 std::copy(rotatedView,rotatedView+12,g_nativeSceneViewMatrix);actor={};::capture_current_light(actor);
 assert(actor.lightVector[0]==0 && actor.lightVector[2]==4096 && actor.lightColor[2]==0x33 && actor.additionalLight.vector[0]==4096);
 g_nativeSceneViewValid=false;
 pointLightParams[0]=100*65536;actor={};::capture_current_light(actor);
 assert(!actor.additionalLight.active && actor.lightVector[0]==4096 && actor.lightColor[2]==0x33);
 loc_601FA9E=0x6103;actor={};::capture_current_light(actor);
 assert(actor.lightVector[0]==700 && actor.lightColor[0]==6 && !actor.additionalLight.active);
 begin_frame(false);assert(!g_fieldActorBaseLight.active);
 puts("PASS: production environment snapshot/application/frame reset, copied color/range, native matrix, outside-range preservation and actor/UI exclusion; GPU/device untested");
}

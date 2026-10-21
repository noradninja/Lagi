#include "lagi/native_scene_mode.h"
#include <cassert>
#include <cstdint>
#include <cstring>
#include <cstdio>
using s32=std::int32_t;
struct {struct{s32 m10_nearClipDistance,m14_farClipDistance;}m405C;}graphicEngineStatus{};
bool hasView=true;int viewWrites=0,cameraWrites=0;unsigned publishedMode=0;
float nearClip=0,farClip=0;
std::int32_t authored[12]={0,65536,0,1234567,-65536,0,0,-1234567,0,0,65536,7654321},published[12]{};
namespace lagi::azel_bridge {
bool native_scene_view_matrix(std::int32_t out[12]){if(!hasView)return false;std::memcpy(out,authored,sizeof(authored));return true;}
}
namespace lagi::platform::renderer {
void presentation_set_scene_mode(unsigned mode){publishedMode=mode;}
void presentation_set_clip_planes(float a,float b){nearClip=a;farClip=b;}
void presentation_set_native_view_matrix(const std::int32_t in[12]){++viewWrites;std::memcpy(published,in,sizeof(published));}
void presentation_set_camera(const float a[3],const float b[3],const float c[3],const float d[3],float x,float y,float z){
    ++cameraWrites;assert(a[0]==0 && a[1]==0 && a[2]==0 && b==a);
    assert(c[0]==0 && c[1]==0 && c[2]==1 && d[0]==0 && d[1]==1 && d[2]==0);
    assert(x==0 && y==0 && z==0);
}
}
#include "service.inc"
int main(){
    for(int mode=-10;mode<20;++mode){
        assert(lagi::native_scene_mode(mode)==(mode>=1 && mode<=4));
        assert(lagi::native_scene_depth_mode(mode)==(mode>=1 && mode<=4));
    }
    graphicEngineStatus.m405C={0x18000,0x12340000};
    sync_battle_presentation_state();
    assert(publishedMode==4 && nearClip==1.5f && farClip==4660.0f);
    assert(viewWrites==1 && cameraWrites==1 && !std::memcmp(authored,published,sizeof(authored)));
    hasView=false;sync_battle_presentation_state();assert(viewWrites==1 && cameraWrites==2);
    puts("PASS: native scene mode capability, authored battle view/clip publication and neutral fallback; mocked renderer, not task or GPU execution");
}

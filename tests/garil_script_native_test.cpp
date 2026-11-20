#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <vector>
using s8=std::int8_t;using s16=std::int16_t;using s32=std::int32_t;using u32=std::uint32_t;
struct File {} a5,a3;
File* gFLD_A5=&a5;
struct sSaturnPtr {u32 m_offset;File* m_file;};
struct Fade {u32 m0_color;};
struct {Fade m0_fade0{0x1234},m24_fade1{0x5678};unsigned m_4C=0,m_4D=0,m_48=0;} g_fadeControls;
struct {struct {unsigned m112_CLOFSL=0;} m20_registers[2];bool m_isDirty=false;} vdp2Controls;
struct Call {Fade* fade;u32 from;s32 to,duration;};
std::vector<Call> calls;
u32 convertColorToU32ForFade(u32 color){return color^0x1357;}
void fadePalette(Fade* fade,u32 from,s32 to,s32 duration){calls.push_back({fade,from,to,duration});}
struct s_workArea {virtual ~s_workArea()=default;struct Task {bool finished=false;void markFinished(){finished=true;}} task;Task* getTask(){return &task;}};
struct s_cutsceneTask2 : s_workArea {u32 m0=0;} cutscene;
struct s_cutsceneTask3 : s_workArea {s32 m0=0;} cutscene3;
struct s_cutsceneTask : s_workArea {u32 m0=0;} cutscene1;
struct Script {s_workArea* m48_cutsceneTask=nullptr;s32 m5C=0;void* m78=nullptr;} script;
struct Dragon {unsigned char m249_noCollisionAndHideDragon=0;void* mB8_lightWingEffect=nullptr;int unrelated=123;} dragon;
struct SubField {Script* m34C_ptrToE=&script;Dragon* m338_pDragonTask=&dragon;} subfield;
struct Field {SubField* m8_pSubFieldData=&subfield;} field;
Field* getFieldTaskPtr(){return &field;}
int restores=0,cuts=0;
void endCutsceneCameraWithRestore(){++restores;}
void endCutsceneCameraNoRestore(){++cuts;}
bool placementReady=false;
bool lagiAzelTryGarilPlacement(sSaturnPtr target){return placementReady&&target.m_file==gFLD_A5&&target.m_offset==0x0605C020;} // separately tested producer
#include "production.inc"
int main(){
 s32 result=99;
 placementReady=true;
 assert(lagiAzelTryGarilScriptAction({0x0605C020,&a5},result)&&result==0);
 placementReady=false;result=99;
 for(u32 address:{0x0607C82Au,0x0607C86Cu}){
  dragon.m249_noCollisionAndHideDragon=17;
  assert(!lagiAzelTryGarilScriptAction({address,&a3},result)&&result==99&&dragon.m249_noCollisionAndHideDragon==17);
  dragon.mB8_lightWingEffect=&dragon;
  assert(!lagiAzelTryGarilScriptAction({address,&a5},result)&&result==99&&dragon.m249_noCollisionAndHideDragon==17);
  dragon.mB8_lightWingEffect=nullptr;
  assert(lagiAzelTryGarilScriptAction({address,&a5},result)&&result==0);
  assert(dragon.m249_noCollisionAndHideDragon==(address==0x0607C82A?1:0)&&dragon.unrelated==123);
  result=99;
 }
 for(auto target:{sSaturnPtr{0x060728E0,&a3},sSaturnPtr{0x060728A4,nullptr},sSaturnPtr{0x0606FA5C,&a5}})
  assert(!lagiAzelTryGarilScriptAction(target,result)&&result==99);
 gFLD_A5=nullptr;assert(!lagiAzelTryGarilScriptAction({0x060728E0,&a5},result));gFLD_A5=&a5;
 assert(lagiAzelTryGarilScriptAction({0x060728F8,&a5},result)&&result==0&&script.m5C==1);
 script.m48_cutsceneTask=&cutscene;
 assert(lagiAzelTryGarilScriptAction({0x060728A4,&a5},result)&&!cutscene.task.finished);
 assert(lagiAzelTryGarilScriptAction({0x060728E0,&a5},result)&&script.m5C==0);
 assert(lagiAzelTryGarilScriptAction({0x060728A4,&a5},result)&&cutscene.task.finished&&cuts==1&&!restores);
 assert(!script.m48_cutsceneTask&&script.m5C==0);
 script.m48_cutsceneTask=&cutscene3;script.m5C=-1;
 lagiAzelTryGarilScriptAction({0x060728A4,&a5},result);assert(!cutscene3.task.finished);
 script.m5C=0;script.m48_cutsceneTask=nullptr;
 lagiAzelTryGarilScriptAction({0x060728A4,&a5},result);assert(cuts==1&&!restores);
 field.m8_pSubFieldData=nullptr;
 lagiAzelTryGarilScriptAction({0x060728A4,&a5},result);assert(cuts==1&&!restores);
 field.m8_pSubFieldData=&subfield;result=99;
 script.m48_cutsceneTask=&cutscene3;cutscene3.m0=2;
 assert(lagiAzelTryGarilScriptAction({0x060728A4,&a5},result)&&cutscene3.task.finished&&restores==1);
 script.m48_cutsceneTask=&cutscene1;script.m78=&script;cutscene1.m0=2;
 assert(lagiAzelTryGarilScriptAction({0x060728A4,&a5},result)&&cutscene1.task.finished&&restores==1&&cuts==1);
 s_workArea foreign;script.m48_cutsceneTask=&foreign;result=99;
 assert(!lagiAzelTryGarilScriptAction({0x060728A4,&a5},result)&&result==99&&!foreign.task.finished);
 assert(script.m48_cutsceneTask==&foreign);script.m48_cutsceneTask=nullptr;result=99;
 for(auto target:{sSaturnPtr{0x0606FA5C,&a3},sSaturnPtr{0x0606FAB4,nullptr},sSaturnPtr{0x060728E0,&a5}}){
  assert(!lagiAzelTryGarilScriptAction(target,15,result)&&result==99&&calls.empty());
 }
 gFLD_A5=nullptr;assert(!lagiAzelTryGarilScriptAction({0x0606FA5C,&a5},15,result));gFLD_A5=&a5;
 for(const auto pair:{std::array<unsigned,2>{0,0},{128,127},{127,128},{255,0},{0,255}})
 for(u32 address:{0x0606FA5Cu,0x0606FAB4u})
 for(s32 duration:{0,1,15,-1,INT32_MAX,INT32_MIN}){
  calls.clear();result=99;g_fadeControls.m_4C=pair[0];g_fadeControls.m_4D=pair[1];
  g_fadeControls.m_48=0xFFFF;
  for(auto& r:vdp2Controls.m20_registers)r.m112_CLOFSL=3;vdp2Controls.m_isDirty=false;
  assert(lagiAzelTryGarilScriptAction({address,&a5},duration,result)&&result==0&&calls.size()==2);
  const bool selected=s8(pair[1])>=s8(pair[0]);
  assert(vdp2Controls.m_isDirty==selected);
  for(auto& r:vdp2Controls.m20_registers)assert(r.m112_CLOFSL==(selected?0x10u:3u));
  assert(calls[0].fade==&g_fadeControls.m0_fade0&&calls[1].fade==&g_fadeControls.m24_fade1);
  for(const auto& call:calls){
   assert(call.duration==s32(u32(duration)<<1));
   assert(call.from==(address==0x0606FA5C?(call.fade->m0_color^0x1357):0x8000));
   assert(call.to==(address==0x0606FA5C?0x8000:-1));
  }
 }
 puts("PASS A5 owned actions, typed cutscene retirement/restore guards, script state, fade groups/colors/duration; native camera/fade services mocked");
}

#include <array>
#include <cassert>
#include <vector>
#include <cstdint>
#include <cstring>
#include <cstdio>
using p_workArea=void*;using s_workAreaCopy=void;using s32=int;
template<class T> struct s_workAreaTemplate {struct Task {bool finished=false;void markFinished(){finished=true;}} task;Task* getTask(){return &task;}struct TypedTaskDefinition {void(*init)(T*);void(*update)(T*);void(*draw)(T*);void(*destroy)(T*);};};
template<class T> void* createSubTask(void*){auto* task=new T;T::getTypedTaskDefinition()->init(task);return task;}
struct {int m4_gameStatus=0x4F,m0_gameMode=6,m8_nextGameStatus=0;} gGameStatus;
using u32=std::uint32_t;using u16=std::uint16_t;
struct sSaturnMemoryFile {
 std::uint8_t* m_data=nullptr;
 u32 m_dataSize=0,m_base=0;
 u32 getSaturnPtr(u32 address){return address;}
};
struct {
 std::uint8_t bitField[32]{};
 bool storyAllowed=true;
 unsigned readPackedBits(int,int){return 6;}
 bool getBit(int){return storyAllowed;}
} mainGameState;
std::uint8_t townBuffer[0xB0000];
int allocatorDepth=0,vdp1Depth=0;
bool findFileOnDisc(const char*){return true;}
void resetTempAllocators(){}
void initDramAllocator(void*,std::uint8_t*,u32,const char**){++allocatorDepth;}
void initVdp1Ram(void*,u32,u32){++vdp1Depth;}
void vdp1FreeLastAllocation(void*){assert(vdp1Depth==1);--vdp1Depth;}
void* allocateNPC(void* task,int){return task;}
void freeRamResources(void*){assert(allocatorDepth==1);--allocatorDepth;}
using s16=std::int16_t;
struct fixedPoint {int value=0;fixedPoint(int n=0):value(n){}int asS32()const{return value;}static int fromInteger(int value){return value*65536;}};
using sVec3_FP=std::array<fixedPoint,3>;
int MTH_Mul(fixedPoint a,int b){return int((std::int64_t(a.value)*b)>>16);}
struct {fixedPoint m[3][3],Mx,My,Mz;} gVdp2RotationMatrix;
int intDivide(int a,int b){return a/b;}
namespace lagi::worldmap {
struct MapDisplayInput {u32 program=0,targetTable=0;int targetCount=0,cameraSelection=0;bool arrivalReady=false,arrivalBusy=false;sVec3_FP arrivalPosition{},arrivalRotation{};};
struct MapDestination {int gameStatus=0,type=0,storyBit=0;bool canTravel()const{return gameStatus>0;}};
MapDestination selectedDestination;
MapDestination read_map_destination(u32){return selectedDestination;}
bool preparationSucceeds=true;int preparationCalls=0;
bool prepare_map_travel(const MapDestination&){++preparationCalls;return preparationSucceeds;}
struct MapDisplay {struct Marker {unsigned flags=0;} marker;bool seeded=false;int state=0,selectedTarget=0;sVec3_FP camera{},rotation{};};
void apply_map_orbit_buttons(MapDisplay&,u16){}
struct MapArrivalAnimation {bool ready=false;};
struct MapMessage {int mode=3,queuedMode=-1;};
void request_map_message(MapMessage&,int){}
bool messageCameraReady=false;int messageUpdates=0;
bool update_map_message(MapMessage&,void*,u32,u32,int,int,bool){assert(messageCameraReady);messageCameraReady=false;++messageUpdates;return true;}
bool initialize_map_arrival_animation(MapArrivalAnimation& state,void*){state.ready=true;return true;}
int animationUpdates=0;
std::vector<int> childUpdateOrder;
void update_map_arrival_animation(MapArrivalAnimation&){++animationUpdates;childUpdateOrder.push_back(1);}
void draw_map_arrival(const MapArrivalAnimation&,MapDisplayInput&,int){}
int tableToggles=0;
bool toggle_map_destinations(MapDisplayInput& in,MapDisplay& display){
 ++tableToggles;in.targetTable=0x06059688;in.targetCount=33;display.selectedTarget=32;return true;
}
bool initialize_map_model(MapDisplay::Marker&,void* owner,u32 light){assert(owner&&light==0x06058774);return true;}
bool initialize_map_arrival(MapDisplayInput&,int){return true;}
bool retarget_map_arrival(MapDisplayInput&,int,fixedPoint&,int& distance,int&){distance=0x6000;return true;}
int arrivalMotionCalls=0;
void update_map_arrival_heading(MapDisplayInput& input,fixedPoint,int){childUpdateOrder.push_back(2);input.arrivalRotation[1]=123;}
bool lastDisplayBusy=false;
void update_map_arrival_motion(MapDisplayInput& input,fixedPoint yaw,int,int,int&,int&){
 assert(yaw.asS32()==123); // movement receives live eased yaw, not the desired heading
 childUpdateOrder.push_back(3);
 ++arrivalMotionCalls;input.arrivalBusy=true;
}
bool update_map_display(MapDisplay& display,const MapDisplayInput& input){
 assert(input.program==0x06054000);
 lastDisplayBusy=input.arrivalBusy;
 if(display.state==4)display.state=1;
 return display.seeded=true;
}
bool initialize_map_destinations(MapDisplayInput& input,u32 program,bool fleet,unsigned route){
 assert(fleet&&route==6);
 input.program=program;
 return true;
}
}
namespace lagi::worldmap {
int find_map_destination(const MapDisplayInput&,int selected,unsigned){return selected;}
void apply_map_camera_and_light(const MapDisplay&){messageCameraReady=true;}
void draw_map_model(MapDisplay::Marker&){}
 bool draw_map_trail(const MapDisplay&,s_workAreaCopy*){return true;}
 struct MapGround {};
 void initialize_map_ground(MapGround&){}
 bool draw_map_ground(MapGround&,u32,s_workAreaCopy*){return true;}
}
namespace lagi { bool worldEnabled=false;bool native_scene_mode(int mode){return mode==6&&worldEnabled;} }
namespace lagi::azel_bridge { void capture_native_scene_view_matrix(){} void begin_view_relative_submission_scope(){} void end_view_relative_submission_scope(){} }
struct Pad {unsigned mC_newButtonDown2=0,m8_newButtonDown=0;};
struct Projection {
 int m10_nearClipDistance=0,m30_oneOverNearClip=0,m14_farClipDistance=0;
 int m38_oneOverFarClip=0,m34_oneOverFarClip256=0;
};
struct {struct {struct {Pad m0_current;} m0_inputDevices[1];} m4514;Projection m405C;} graphicEngineStatus;
void reset3dEngine(){}
int FP_Div(int a,int b){return b?int((std::int64_t(a)*65536)/b):(a>=0?INT32_MAX:INT32_MIN);}
fixedPoint FP_Div(int a,fixedPoint b){return FP_Div(a,b.asS32());}
fixedPoint getSin(unsigned){return 0;}
std::vector<int> sounds;
void playSystemSoundEffect(int sound){sounds.push_back(sound);}
struct Fade {u32 m0_color=0;int m20_stopped=0;};
struct {Fade m0_fade0;int m_48=0;std::uint8_t m_4C=0,m_4D=0;} g_fadeControls;
u32 convertColorToU32ForFade(u32 color){return color;}
int fadeCalls=0,fadeFrames=0;u32 fadeTarget=0;
void fadePalette(Fade* fade,u32,u32 target,int frames){++fadeCalls;fadeFrames=frames;fadeTarget=target;fade->m20_stopped=0;}
void setNextGameStatus(int status){assert(!gGameStatus.m8_nextGameStatus);gGameStatus.m8_nextGameStatus=status;}
struct Vdp2Regs {u32 m70_SCXN0=0,m74_SCYN0=0,mE_RAMCTL=0,m10_CYCA0=0,m38_PNCR=0,mBC_RPTAU=0,mBE_RPTAL=0,mC0_WPSX0=0,mC2_WPSY0=0,mC4_WPEX0=0,mC6_WPEY0=0,mA8_LCTA=0,mAC_BKTA=0,mF0_PRISA=0,mF2_PRISB=0,mF4_PRISC=0,mF6_PRISD=0,mF8_PRINA=0,mFA_PRINB=0,mFC_PRIR=0;};
struct {Vdp2Regs* m4_pendingVdp2Regs=nullptr;struct {u32 m112_CLOFSL=0;}m20_registers[2];bool m_isDirty=false;int m0_doubleBufferIndex=0;} vdp2Controls;
enum LayerCommand {m0_END,m2_CHCN,m5_CHSZ,m6_PNB,m7_CNSM,m11_SCN,m27_RPMD,m31_RxKTE,m34_W0E,m37_W0A,m44_CCEN,m45_COEN};
struct sLayerConfig {LayerCommand command;u32 value;};
Vdp2Regs worldRegs;int vdp2Resets=0;
void reinitVdp2(){++vdp2Resets;worldRegs={};vdp2Controls.m4_pendingVdp2Regs=&worldRegs;}
void setupNBG0(const sLayerConfig*){}
void setupRGB0(const sLayerConfig*){}
void setupRotationParams(const sLayerConfig*){}
void setupRotationMapPlanes(int,u32){}
std::vector<fixedPoint> coefficientA0(512),coefficientA1(512);
std::vector<fixedPoint>* gVdp2CoefficientTables[2][2]{};
void setupVdp2Table(int,std::vector<fixedPoint>& a,std::vector<fixedPoint>& b,std::uint8_t*,int){
 gVdp2CoefficientTables[0][0]=&a;gVdp2CoefficientTables[0][1]=&b;
 for(auto& value:a)value=0x10000;
 for(auto& value:b)value=0x10000;
}
void initRotationCoefficientTables(int,std::uint8_t*){}
void initLayerMap(u32,u32,u32,u32,u32){}
void getVdp1ClippingCoordinates(std::array<s16,4>& out){out={0,0,351,223};}
void getVdp1ProjectionParams(s16* x,s16* y){*x=176;*y=112;}
void beginRotationPass(int,int){}
void buildRotationMatrixPitchYaw(fixedPoint,fixedPoint){gVdp2RotationMatrix.m[2][1]=0x10000;}
void scaleRotationMatrix(fixedPoint){}
void writeRotationParams(fixedPoint){}
void drawCinematicBar(int){}
void commitRotationPass(){}
struct RotationEntry {int m8_Zst=0;s16 m34,m36,m38,m3C,m3E,m40;};
RotationEntry gCoefficientTables[2][2]{};
std::array<std::uint8_t,0x80000> vram{};
std::uint8_t* getVdp2Vram(u32 offset){return vram.data()+offset;}
std::uint8_t* getVdp2Cram(u32){return nullptr;}
void asyncDmaCopy(u32,std::uint8_t*,int,int){}
u32 readSaturnU32(u32){return 0;}
sVec3_FP readSaturnVec3(u32){return {};}
namespace lagi::worldmap {int map_approach_profile(int,int target,int){return target;}}
void loadFnt(const char*){}
void unloadFnt(){}
void setupVDP2StringRendering(int,int,int,int){}
struct {int m0=0;} vdp2StringContext;
void drawStringSmallFont(const char*){}
void clearVdp2TextArea(){}
bool available=true;int reads=0;
bool backgroundsAvailable=true;
namespace lagi::disc {
int preparationClears=0;
void clear_prepared_native_files(){++preparationClears;}
bool read_file(const char* name,std::vector<std::uint8_t>& out){
 if(!std::strcmp(name,"WORLD.PRG")){
  ++reads;
  if(available)out.assign(0x5ED4,0);
  return available;
 }
 if(!backgroundsAvailable && !std::strcmp(name,"FRS_WLD.PNB"))return false;
 out.assign(8,0x5A);
 return true;
}
}
namespace lagi::platform::logging {template<class... T>void writef(const char*,T...) {}}
#include "production.inc"
int main(){int parent;mainGameState.bitField[0x1B]=8;
 for(const auto pair : {std::array<int,2>{0,0},{128,127},{127,128}}){
  g_fadeControls.m_4C=std::uint8_t(pair[0]);g_fadeControls.m_4D=std::uint8_t(pair[1]);
  for(auto& registers:vdp2Controls.m20_registers)registers.m112_CLOFSL=0x10;
  LagiWorldBoundary::BeginSelectionFadeIn();
  const bool cleared=std::int8_t(pair[0])<=std::int8_t(pair[1]);
  for(const auto& registers:vdp2Controls.m20_registers)assert(registers.m112_CLOFSL==(cleared?0u:0x10u));
  assert(fadeFrames==30&&fadeTarget==u32(g_fadeControls.m_48));
 }
 g_fadeControls.m_4C=g_fadeControls.m_4D=0;
 for(bool present:{true,false}){available=present;auto* task=static_cast<LagiWorldBoundary*>(lagiLoadNativeWorldBoundary(&parent,0));
  assert(task&&static_cast<void*>(task)!=&parent);assert(task->program.size()==(present?0x5ED4:0));
  assert(task->displayReady==present&&task->display.seeded==present);
  if(present)assert(task->worldFile.m_data==task->program.data()&&task->destinations.program==0x06054000);
  assert(gGameStatus.m4_gameStatus==0x4F&&gGameStatus.m0_gameMode==6);task->getTypedTaskDefinition()->destroy(task);assert(!allocatorDepth&&!vdp1Depth);delete task;}
 assert(reads==2);
 lagi::worldEnabled=true;
 available=true;
 for(bool present:{false,true}){
  backgroundsAvailable=present;
  vram.fill(0);
  vdp2Resets=0;
  auto* task=static_cast<LagiWorldBoundary*>(lagiLoadNativeWorldBoundary(&parent,0));
  assert(task->displayReady==present && vdp2Resets==int(present));
  for(u32 offset:{0x10000u,0x1E000u,0x40000u,0x60000u})
   assert(vram[offset]==(present?0x5A:0));
  if(present){
   assert(task->display.marker.flags&0x20);
   assert(task->display.marker.flags&0x40);
   assert(worldRegs.mF8_PRINA==0x0602 && worldRegs.mFC_PRIR==5);
   assert(graphicEngineStatus.m405C.m10_nearClipDistance==0x800);
   assert(graphicEngineStatus.m405C.m14_farClipDistance==0x400000);
   task->display.rotation[1]=0x1000000;
   const auto originalCamera=task->display.camera;
   for(int buffer:{0,1}){
    vdp2Controls.m0_doubleBufferIndex=buffer;
    const auto phase=task->environment.wave.phase;
    LagiWorldBoundary::Draw(task);
    assert(task->environment.wave.phase==phase+0x11102Au);
    assert(worldRegs.m70_SCXN0==0xFFC00000u);
    assert(worldRegs.m74_SCYN0==0xFE560000u);
    assert(worldRegs.mFC_PRIR==3);
    for(unsigned axis=0;axis<3;++axis)
     assert(task->display.camera[axis].asS32()==originalCamera[axis].asS32());
   }
   task->display.camera[1]=task->environment.planeHeight+1;
   LagiWorldBoundary::Draw(task);
   assert(worldRegs.mFC_PRIR==5);
   vdp2Controls.m0_doubleBufferIndex=0;
  }
  LagiWorldBoundary::Delete(task);
  assert(!allocatorDepth&&!vdp1Depth);
  delete task;
 }
 auto& pad=graphicEngineStatus.m4514.m0_inputDevices[0].m0_current;
 for (int type : {0,1,2,3}) {
  fadeCalls=0;lagi::worldmap::arrivalMotionCalls=0;
  lagi::worldmap::preparationCalls=0;
  LagiWorldBoundary task;
  task.displayReady=true;
  task.display.state=1;
  task.destinations.program=0x06054000;
  task.destinations.arrivalReady=true;
  lagi::worldmap::selectedDestination={0x52,type,0};
  pad.m8_newButtonDown=2;
  for(int n=0;n<15;++n)LagiWorldBoundary::Update(&task);
  assert(task.travelStatus<0);
  // An authored disabled destination cannot enter either travel path.
  lagi::worldmap::selectedDestination.gameStatus=0;
  LagiWorldBoundary::Update(&task);
  assert(task.travelStatus<0 && task.display.state==1);
  lagi::worldmap::selectedDestination.gameStatus=0x52;
  // Keep original story availability authoritative, independently of type.
  lagi::worldmap::selectedDestination.storyBit=193;
  mainGameState.storyAllowed=false;
  sounds.clear();
  LagiWorldBoundary::Update(&task);
  assert(task.travelStatus<0 && task.display.state==1);
  assert(sounds==std::vector<int>{5});
  assert(lagi::worldmap::preparationCalls==0);
  mainGameState.storyAllowed=true;
  if(type<2){
   task.destinations.arrivalReady=false;
   LagiWorldBoundary::Update(&task);
   assert(task.travelStatus<0 && task.display.state==1);
   task.destinations.arrivalReady=true;
  }
  // Failed preparation cannot initiate movement, fades or module dispatch.
  lagi::worldmap::preparationSucceeds=false;
  sounds.clear();
  LagiWorldBoundary::Update(&task);
  assert(task.travelStatus<0 && task.display.state==1 && !fadeCalls);
  assert(lagi::worldmap::preparationCalls==1);
  assert(sounds.empty());
  lagi::worldmap::preparationSucceeds=true;
  lagi::worldmap::childUpdateOrder.clear();
  LagiWorldBoundary::Update(&task);
  assert(task.travelStatus==0x52 && task.display.state==(type<2?3:2));
  assert(lagi::worldmap::preparationCalls==2);
  assert(sounds==std::vector<int>{0});
  assert(!fadeCalls);
  if(type<2){
   assert(task.destinations.arrivalBusy && !lagi::worldmap::lastDisplayBusy);
   assert(lagi::worldmap::arrivalMotionCalls==1);
   assert((lagi::worldmap::childUpdateOrder==std::vector<int>{1,2,3}));
  }else assert(!lagi::worldmap::arrivalMotionCalls);
  pad.m8_newButtonDown=0;
  fadeCalls=0;
  const int delay=type<2?1:15;
  for(int n=1;n<delay;++n)LagiWorldBoundary::Update(&task);
  assert(!fadeCalls);
  LagiWorldBoundary::Update(&task);
  assert(fadeCalls==1 && fadeTarget==0x8000 && fadeFrames==(type<2?60:30));
  if(type<2){
   assert(lagi::worldmap::lastDisplayBusy);
   assert(lagi::worldmap::arrivalMotionCalls==2);
  }
  LagiWorldBoundary::Update(&task);
  assert(!task.task.finished && !gGameStatus.m8_nextGameStatus);
  g_fadeControls.m0_fade0.m20_stopped=1;
  gGameStatus.m8_nextGameStatus=0x50;
  LagiWorldBoundary::Update(&task);
  assert(!task.task.finished && gGameStatus.m8_nextGameStatus==0x50);
  gGameStatus.m8_nextGameStatus=0;
  LagiWorldBoundary::Update(&task);
  assert(task.task.finished && gGameStatus.m8_nextGameStatus==0x52);
  assert(task.preparedFilesHandedOff);
  const auto clears=lagi::disc::preparationClears;
  LagiWorldBoundary::Delete(&task);
  assert(lagi::disc::preparationClears==clears);
  gGameStatus.m8_nextGameStatus=0;
 }
 // Authored negative status fades and returns within WORLD. It must never
 // prepare a destination or dispatch a negative game status as a module.
 {
  LagiWorldBoundary task;task.displayReady=true;task.selectionDelay=0;
  task.display.state=1;task.destinations.program=0x06054000;
  lagi::worldmap::selectedDestination={-1,0,193};
  lagi::worldmap::preparationCalls=0;fadeCalls=0;
  mainGameState.storyAllowed=false;pad.m8_newButtonDown=2;
  LagiWorldBoundary::Update(&task);
  assert(!task.waitingForMapToggle&&!fadeCalls);
  mainGameState.storyAllowed=true;
  LagiWorldBoundary::Update(&task);
  assert(task.waitingForMapToggle&&fadeCalls==1&&fadeFrames==30&&fadeTarget==0);
  assert(!lagi::worldmap::preparationCalls&&!lagi::worldmap::tableToggles);
  for(int i=0;i<5;++i)LagiWorldBoundary::Update(&task);
  assert(fadeCalls==1&&task.waitingForMapToggle&&!lagi::worldmap::tableToggles);
  g_fadeControls.m0_fade0.m20_stopped=1;
  LagiWorldBoundary::Update(&task);
  assert(!task.waitingForMapToggle&&lagi::worldmap::tableToggles==1);
  assert(task.display.selectedTarget==32&&task.destinations.targetCount==33&&task.display.state==1);
  assert(fadeCalls==2&&fadeFrames==30&&fadeTarget==u32(g_fadeControls.m_48));
  assert(task.selectionDelay==15&&!task.task.finished&&!gGameStatus.m8_nextGameStatus);
  assert(!lagi::worldmap::preparationCalls&&!task.preparedFilesHandedOff);
 }
 puts("PASS WORLD startup ownership, both environment buffers, scroll/priority writes, camera isolation, four destination types, story/readiness guards, entry delay, fades, and module handoff");
}

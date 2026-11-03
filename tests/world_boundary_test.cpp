#include <array>
#include <cassert>
#include <vector>
#include <cstdint>
#include <cstring>
#include <cstdio>
using p_workArea=void*;using s32=int;
template<class T> struct s_workAreaTemplate {struct Task {void markFinished(){}} task;Task* getTask(){return &task;}struct TypedTaskDefinition {void(*init)(T*);void(*update)(T*);void(*draw)(T*);void(*destroy)(T*);};};
template<class T> void* createSubTask(void*){auto* task=new T;T::getTypedTaskDefinition()->init(task);return task;}
struct {int m4_gameStatus=0x4F,m0_gameMode=6,m8_nextGameStatus=0;} gGameStatus;
using u32=std::uint32_t;
struct sSaturnMemoryFile {
 std::uint8_t* m_data=nullptr;
 u32 m_dataSize=0,m_base=0;
 u32 getSaturnPtr(u32 address){return address;}
};
struct {
 std::uint8_t bitField[32]{};
 unsigned readPackedBits(int,int){return 6;}
 bool getBit(int){return true;}
} mainGameState;
std::uint8_t townBuffer[0xB0000];
int allocatorDepth=0;
bool findFileOnDisc(const char*){return true;}
void resetTempAllocators(){}
void initDramAllocator(void*,std::uint8_t*,u32,const char**){++allocatorDepth;}
void initVdp1Ram(void*,u32,u32){}
void* allocateNPC(void* task,int){return task;}
void freeRamResources(void*){assert(allocatorDepth==1);--allocatorDepth;}
using s16=std::int16_t;
struct fixedPoint {int value=0;fixedPoint(int n=0):value(n){}int asS32()const{return value;}static int fromInteger(int value){return value*65536;}};
using sVec3_FP=std::array<fixedPoint,3>;
int MTH_Mul(fixedPoint a,int b){return int((std::int64_t(a.value)*b)>>16);}
struct {fixedPoint m[3][3],Mx,My,Mz;} gVdp2RotationMatrix;
int intDivide(int a,int b){return a/b;}
namespace lagi::worldmap {
struct MapDisplayInput {u32 program=0,targetTable=0;int targetCount=0,cameraSelection=0;bool arrivalReady=false,arrivalBusy=false;};
struct MapDestination {int gameStatus=0,type=0,storyBit=0;bool canTravel()const{return false;}};
MapDestination read_map_destination(u32){return {};}
struct MapDisplay {struct Marker {unsigned flags=0;} marker;bool seeded=false;int state=0,selectedTarget=0;sVec3_FP camera{},rotation{};};
bool initialize_map_model(MapDisplay::Marker&,void* owner,u32 light){assert(owner&&light==0x06058774);return true;}
bool initialize_map_arrival(MapDisplayInput&,int){return true;}
bool retarget_map_arrival(MapDisplayInput&,int,fixedPoint&,int&,int&){return false;}
void update_map_arrival_launch(MapDisplayInput&,fixedPoint,int,int&,int&){}
void update_map_arrival_step(u32,int&,bool&){}
bool update_map_display(MapDisplay& display,const MapDisplayInput& input){
 assert(input.program==0x06054000);
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
void apply_map_camera_and_light(const MapDisplay&){}
void draw_map_model(MapDisplay::Marker&){}
}
namespace lagi { constexpr bool native_scene_mode(int){return false;} }
namespace lagi::azel_bridge { void capture_native_scene_view_matrix(){} }
struct Pad {unsigned mC_newButtonDown2=0,m8_newButtonDown=0;};
struct {struct {struct {Pad m0_current;} m0_inputDevices[1];} m4514;} graphicEngineStatus;
void playSystemSoundEffect(int){}
struct Fade {u32 m0_color=0;int m20_stopped=0;};
struct {Fade m0_fade0;int m_48=0;} g_fadeControls;
u32 convertColorToU32ForFade(u32 color){return color;}
void fadePalette(Fade*,u32,u32,int){}
void setNextGameStatus(int){}
struct Vdp2Regs {u32 mE_RAMCTL=0,m10_CYCA0=0,m38_PNCR=0,mBC_RPTAU=0,mBE_RPTAL=0;};
struct {Vdp2Regs* m4_pendingVdp2Regs=nullptr;bool m_isDirty=false;int m0_doubleBufferIndex=0;} vdp2Controls;
enum LayerCommand {m0_END,m2_CHCN,m5_CHSZ,m6_PNB,m7_CNSM,m11_SCN,m27_RPMD,m31_RxKTE,m34_W0E,m37_W0A,m44_CCEN,m45_COEN};
struct sLayerConfig {LayerCommand command;u32 value;};
void reinitVdp2(){}
void setupNBG0(const sLayerConfig*){}
void setupRGB0(const sLayerConfig*){}
void setupRotationParams(const sLayerConfig*){}
void setupRotationMapPlanes(int,u32){}
int coefficientA0=0,coefficientA1=0;
void setupVdp2Table(int,int&,int&,std::uint8_t*,int){}
void initRotationCoefficientTables(int,std::uint8_t*){}
void initLayerMap(u32,u32,u32,u32,u32){}
void getVdp1ClippingCoordinates(std::array<s16,4>&){}
void getVdp1ProjectionParams(s16*,s16*){}
void beginRotationPass(int,int){}
void buildRotationMatrixPitchYaw(fixedPoint,fixedPoint){}
void scaleRotationMatrix(fixedPoint){}
void writeRotationParams(fixedPoint){}
void drawCinematicBar(int){}
void commitRotationPass(){}
struct RotationEntry {s16 m34,m36,m38,m3C,m3E,m40;};
RotationEntry gCoefficientTables[2][2]{};
std::uint8_t* getVdp2Vram(u32){return nullptr;}
std::uint8_t* getVdp2Cram(u32){return nullptr;}
void asyncDmaCopy(u32,std::uint8_t*,int,int){}
u32 readSaturnU32(u32){return 0;}
void loadFnt(const char*){}
void unloadFnt(){}
void setupVDP2StringRendering(int,int,int,int){}
struct {int m0=0;} vdp2StringContext;
void drawStringSmallFont(const char*){}
bool available=true;int reads=0;
namespace lagi::disc {bool read_file(const char* name,std::vector<std::uint8_t>& out){assert(!std::strcmp(name,"WORLD.PRG"));++reads;if(available)out.assign(0x5ED4,0);return available;}}
namespace lagi::platform::logging {template<class... T>void writef(const char*,T...) {}}
#include "production.inc"
int main(){int parent;mainGameState.bitField[0x1B]=8;
 for(bool present:{true,false}){available=present;auto* task=static_cast<LagiWorldBoundary*>(lagiLoadNativeWorldBoundary(&parent,0));
  assert(task&&static_cast<void*>(task)!=&parent);assert(task->program.size()==(present?0x5ED4:0));
  assert(task->displayReady==present&&task->display.seeded==present);
  if(present)assert(task->worldFile.m_data==task->program.data()&&task->destinations.program==0x06054000);
  assert(gGameStatus.m4_gameStatus==0x4F&&gGameStatus.m0_gameMode==6);task->getTypedTaskDefinition()->destroy(task);assert(!allocatorDepth);delete task;}
 assert(reads==2);puts("PASS production WORLD boundary owns a real child/program, preserves native status, handles missing file without an intro fallback; map display remains unimplemented");
}

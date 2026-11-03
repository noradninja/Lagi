#include <array>
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
using u8=std::uint8_t;using u16=std::uint16_t;using s16=std::int16_t;using u32=std::uint32_t;
struct sVec3_FP {};
struct s_workArea {virtual ~s_workArea()=default;} owner;
template<class T>struct s_workAreaTemplate:s_workArea {
 struct TypedTaskDefinition {void(*init)(T*);void(*update)(T*);void(*draw)(T*);void(*destroy)(T*);};
};
bool allocationFails=false;
template<class T>T* createSubTask(s_workArea* parent){assert(parent==&owner);return allocationFails?nullptr:new T;}
struct sSaturnPtr {const char* text=nullptr;bool isNull()const{return !text;}};
std::string readSaturnString(sSaturnPtr p){return p.text;}
int columns=3;
int computeStringLength(sSaturnPtr,int limit){assert(limit==40);return columns;}
struct Context {int m0=0,m4_cursorX=1,m8_cursorY=2,mC_X=3,m10_Y=4,m14_Width=5,m18_Height=6;} vdp2StringContext;
std::array<u8,0x80000> vram{},vdp1{};
u16 readCell(u32 offset){return (u16(vram.at(offset))<<8)|vram.at(offset+1);}
void cell(u32 offset,u16 value){vram.at(offset)=u8(value>>8);vram.at(offset+1)=u8(value);}
u16 getVdp2VramU16(u32 offset){return readCell(offset);}
u8* getVdp2Vram(u32 offset){return vram.data()+offset;}
u8* getVdp1Pointer(u32 ea){assert(ea>=0x25C00000&&ea<0x25C80000);return vdp1.data()+ea-0x25C00000;}
void setupVDP2StringRendering(int x,int y,int width,int height){
 assert(x==0&&y==32&&width==columns&&height==2);
 vdp2StringContext.m4_cursorX=x;vdp2StringContext.m8_cursorY=y;
 vdp2StringContext.mC_X=x;vdp2StringContext.m10_Y=y;
 vdp2StringContext.m14_Width=width;vdp2StringContext.m18_Height=height;
}
void VDP2DrawString(const char*){
 for(int i=0;i<columns;++i){
  // Middle column blank only in the short test; upper cell carries palette bits.
  cell(0x7000+i*2,columns==3&&i==1?0:u16(0xC010));cell(0x7080+i*2,0xC011);
 }
 vdp2StringContext.m4_cursorX+=columns;
}
int clears=0,copies=0,invalidations=0;
void clearVdp2TextArea(){
 ++clears;for(int i=0;i<columns;++i){cell(0x7000+i*2,0);cell(0x7080+i*2,0);}
}
void addToMemoryLayout(u8*,u32 size){assert(size==64);}
void memcpy_dma(u8* source,u8* destination,u32 size){assert(size==32);++copies;std::memcpy(destination,source,size);}
u32 invalidatedSize=0;
void invalidateVdp1TextureRange(u32 offset,u32 size){assert(offset==0x7B600);++invalidations;invalidatedSize=size;}
struct Command {u16 m0_CMDCTRL=0,m4_CMDPMOD=0,m6_CMDCOLR=0,m8_CMDSRCA=0,mA_CMDSIZE=0;s16 mC_CMDXA=0,mE_CMDYA=0;};
struct Packet {u16 m4_bucketTypes=0;Command* m6_vdp1EA=nullptr;};
std::array<Command,40> commands{};std::array<Packet,40> packets{};
struct {struct {Command* m0_currentVdp1WriteEA=commands.data();Packet* m20_pCurrentVdp1Packet=packets.data();u32 m1C=0,mC=0;}m14_vdp1Context[1];struct {int m38_oneOverFarClip=512;}m405C;} graphicEngineStatus;
struct fixedPoint {int value;int asS32()const{return value;}};
fixedPoint MTH_Mul(int a,int b){return {int((std::int64_t(a)*b)>>16)};}
#include "lagi/world_map_sprite_text.h"
#include "production.inc"
int main(){
 using namespace lagi::worldmap;
 MapLabelPlacement placement{10,20,0x333};
 assert(!create_map_sprite_text(nullptr,{"A"},&placement));
 assert(!create_map_sprite_text(&owner,{},&placement));
 assert(!create_map_sprite_text(&owner,{"A"},nullptr));
 columns=41;assert(!create_map_sprite_text(&owner,{"A"},&placement));columns=3;
 allocationFails=true;assert(!create_map_sprite_text(&owner,{"A"},&placement));allocationFails=false;
 vdp1.fill(0xEE);
 std::fill(vram.begin()+0x200,vram.begin()+0x220,0x12);
 std::fill(vram.begin()+0x220,vram.begin()+0x240,0x34);
 auto* task=static_cast<MapSpriteTextTask*>(create_map_sprite_text(&owner,{"A B"},&placement));
 assert(task&&task->glyphs.size()==2&&copies==4&&clears==1&&invalidations==1&&invalidatedSize==128);
 assert(vdp1[0x7B600]==0x12&&vdp1[0x7B620]==0x34&&vdp1[0x7B640]==0x12&&vdp1[0x7B680]==0xEE);
 assert(vdp2StringContext.m4_cursorX==1&&vdp2StringContext.m8_cursorY==2&&vdp2StringContext.mC_X==3);
 assert(vdp2StringContext.m10_Y==4&&vdp2StringContext.m14_Width==5&&vdp2StringContext.m18_Height==6);
 assert(readCell(0x7000)==0&&readCell(0x7084)==0);
 MapSpriteTextTask::Draw(task);
 assert(commands[0].m0_CMDCTRL==0x1000&&commands[0].m4_CMDPMOD==0x80&&commands[0].m6_CMDCOLR==0x27C0);
 assert(commands[0].mA_CMDSIZE==0x110&&commands[0].m8_CMDSRCA==(0x7B600>>3));
 assert(commands[0].mC_CMDXA==10&&commands[0].mE_CMDYA==-20&&commands[1].mC_CMDXA==26);
 assert(packets[0].m4_bucketTypes==6&&packets[0].m6_vdp1EA==&commands[0]);
 assert(graphicEngineStatus.m14_vdp1Context[0].m1C==2&&graphicEngineStatus.m14_vdp1Context[0].mC==2);
 placement={-30,40,0x666};MapSpriteTextTask::Draw(task);
 assert(commands[2].mC_CMDXA==-30&&commands[2].mE_CMDYA==-40&&packets[2].m4_bucketTypes==12);
 delete task;
 columns=40;copies=0;invalidations=0;
 task=static_cast<MapSpriteTextTask*>(create_map_sprite_text(&owner,{"forty"},&placement));
 assert(task&&task->glyphs.size()==40&&copies==80&&invalidatedSize==0xA00&&invalidations==1);
 assert(vdp1[0x7BFFF]==0x34&&vdp1[0x7C000]==0xEE);delete task;
 puts("PASS native sprite text extraction/order, scratch-context restoration, reserved-range bounds, live placement, palette, packet depth and task release; no live WORLD cutover yet");
}

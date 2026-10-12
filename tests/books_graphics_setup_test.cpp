#include <cstdint>
#include <array>
#include <vector>
#include <string>
#include <cstdio>
using u32=std::uint32_t; using s32=std::int32_t; using u16=std::uint16_t; using s16=std::int16_t;
enum eVdp2LayerConfig {m0_END=0,m2_CHCN=2,m5_CHSZ=5,m6_PNB=6,m7_CNSM=7,m11_SCN=11,m12_PLSZ=12,m34_W0E=34,m37_W0A=37,m40_CAOS=40};
struct sLayerConfig {eVdp2LayerConfig m_configType;u32 m_value;};
struct Regs {
 u32 m14_CYCA1=0,m1C_CYCB1=0;
 u16 mF8_PRINA=0,mFA_PRINB=0,mFC_PRIR=0,mF0_PRISA=0,mF2_PRISB=0,mF4_PRISC=0,mF6_PRISD=0;
 u16 mC0_WPSX0=0,mC2_WPSY0=0,mC4_WPEX0=0,mC6_WPEY0=0;
} regs;
struct {Regs* m4_pendingVdp2Regs=&regs;u32 m_isDirty=0;} vdp2Controls;
struct Layer {s16 scrollX=0,scrollY=0;};
struct {Layer m40BC_layersConfig[4];} graphicEngineStatus;
struct Common {u32 getSaturnPtr(u32 address){return address;}} common;
Common* gCommonFile=&common;
constexpr u32 tileBytes=0x16000,spriteBytes=0x580;
u32 tilesSize=tileBytes,spritesSize=spriteBytes;
bool failLoad=false;
std::vector<std::string> files;
unsigned setups=0,maps=0,palettes=0,publishes=0,resets=0,windows=0,clears=0;
bool valid=true;
u32 getFileSize(const char* name) {return std::string(name)=="MENUBK2.BIN"?tilesSize:spritesSize;}
std::uint8_t* getVdp2Vram(u32 address){return reinterpret_cast<std::uint8_t*>(std::uintptr_t(0x25E00000u+address));}
std::uint8_t* getVdp1Pointer(u32 address){return reinterpret_cast<std::uint8_t*>(std::uintptr_t(address));}
void* getVdp2Cram(u32 address){return reinterpret_cast<void*>(std::uintptr_t(0x25F00000u+address));}
void table(const sLayerConfig* data,const std::vector<unsigned>& expected) {
 std::vector<unsigned> actual;
 for(;data->m_configType!=m0_END;++data){actual.push_back(data->m_configType);actual.push_back(data->m_value);}
 valid &=actual==expected; ++setups;
}
void setupNBG1(const sLayerConfig* data){table(data,{2,1,5,1,6,1,7,0,11,4,12,0,40,3});}
void setupNBG2(const sLayerConfig* data){table(data,{2,1,5,1,6,1,7,0,11,4,12,0,40,5,35,1,38,1});}
void setupNBG3(const sLayerConfig* data){table(data,{33,1,34,1,37,0,35,1,38,1});}
void initLayerMap(u32 layer,u32 a,u32 b,u32 c,u32 d){valid &=layer==maps+1 && a==0x20000 && a==b && b==c && c==d;++maps;}
int loadFile(const char* name,std::uint8_t* dest,u16 patch){
 files.push_back(name);valid &=patch==0;
 valid &=std::uintptr_t(dest)==(files.size()==1?0x25E20000u:0x25C12000u);
 return failLoad?-1:0;
}
void asyncDmaCopy(u32 source,void* dest,u32 bytes,u32 extra){valid &=source==0x20DDF4 && std::uintptr_t(dest)==0x25F007E0 && bytes==0x420 && extra==0;++palettes;}
void setVdp2LayerScroll(u32 layer,u32 x,u32 y){graphicEngineStatus.m40BC_layersConfig[layer]={s16(x),s16(y+16)};}
void scrollMenu(){++publishes;}
void resetVdp2LayersAutoScroll(){++resets;}
bool lagiAzelBooksPresentWindow(int x,int y,int step,int duration){valid &=x==0 && y==16 && step==0 && duration==1;++windows;return true;}
void setupVDP2StringRendering(int x,int y,int w,int h){valid &=x==0 && y==34 && w==44 && h==28;}
void clearVdp2TextArea(){++clears;}
#include "books_setup_production.inc"
int main(){
 tilesSize=0;
 if(lagiAzelBooksSetupGraphics() || setups || !files.empty())return 1;
 tilesSize=tileBytes+1;
 if(lagiAzelBooksSetupGraphics() || setups || !files.empty())return 2;
 tilesSize=tileBytes;
 if(!lagiAzelBooksSetupGraphics() || !valid || setups!=3 || maps!=2 || palettes!=1 || publishes!=1 || resets!=1 || windows!=1 || clears!=1)return 3;
 if(files!=std::vector<std::string>{"MENUBK2.BIN","MENUBK.CGB"})return 4;
 if(regs.m14_CYCA1!=0x12FF5566 || regs.m1C_CYCB1!=0xffffffff || regs.mF8_PRINA!=0x400 || regs.mFA_PRINB!=0x705 || regs.mF0_PRISA!=0x606 || regs.mF6_PRISD!=0x606 || regs.mC0_WPSX0!=80 || regs.mC2_WPSY0!=352 || regs.mC4_WPEX0!=623 || regs.mC6_WPEY0!=464 || vdp2Controls.m_isDirty!=1)return 5;
 failLoad=true;
 const auto oldClears=clears;
 if(lagiAzelBooksSetupGraphics() || clears!=oldClears)return 6;
 puts("PASS: actual Books graphics setup tables, resource bounds/destinations, palette/scroll/priority/windows and load failure (host mocks, not device rendering)");
}

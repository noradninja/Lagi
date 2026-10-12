#include <cassert>
#include <vector>
#include <cstdio>
#include <cstdint>
#include "lagi/azel_map_menu.h"
using u16=unsigned short;
using u32=unsigned;
using s16=short;
struct {struct Layer {int scrollX=0,scrollY=0;} m40BC_layersConfig[4];} graphicEngineStatus;
unsigned char common[0x21084], vram[0x80000];
struct Common {
 unsigned char* m_data;
 unsigned m_base=0x200000,m_dataSize=0x21084;
 unsigned char* getSaturnPtr(unsigned address){assert(address==0x20A650 || address==0x20A388);return m_data+address-0x200000;}
} commonObject{common};
Common* gCommonFile=&commonObject;
enum eItems:int {};
int readSaturnS16(unsigned char* data){return std::int16_t((unsigned(data[0])<<8)|data[1]);}
struct Pointer {unsigned m_offset;Pointer operator+(int n)const{return {m_offset+unsigned(n)};}};
unsigned char* pointerData(Pointer pointer){assert(pointer.m_offset>=0x200000 && pointer.m_offset<0x221084);return common+pointer.m_offset-0x200000;}
Pointer readSaturnEA(unsigned char* p){return {(unsigned(p[0])<<24)|(unsigned(p[1])<<16)|(unsigned(p[2])<<8)|p[3]};}
Pointer readSaturnEA(Pointer p){return readSaturnEA(pointerData(p));}
int readSaturnS8(Pointer p){return static_cast<signed char>(*pointerData(p));}
int readSaturnS16(Pointer p){return readSaturnS16(pointerData(p));}
unsigned readSaturnU8(Pointer p){return *pointerData(p);}
int readSaturnS32(Pointer p){return static_cast<std::int32_t>(readSaturnEA(p).m_offset);}
int intDivide(int divisor,int value){return value/divisor;}
struct s_menuSprite {short SRCA,SIZE,X,Y;};
unsigned markerDraws=0;int markerSource=0;
void drawMenuSprite(const s_menuSprite* sprite,short x,short y,unsigned color){
 assert(x==128 && y==124 && color==0x760 && sprite->SIZE==0x0108 && sprite->X==-4 && sprite->Y==-4);
 markerSource=sprite->SRCA;++markerDraws;
}
void pointerWord(unsigned offset,unsigned address){for(unsigned i=0;i<4;++i)common[offset+i]=static_cast<unsigned char>(address>>(24-i*8));}
struct Field {int m2C_currentFieldIndex=0;} fieldObject;
Field* fieldPointer=&fieldObject;
Field* getFieldTaskPtr(){return fieldPointer;}
struct {
 int calls=0,last=-1,value=0,flagCalls=0,lastFlag=-1;
 int getItemCount(eItems item){++calls;last=int(item);return value;}
 bool getBit566(unsigned flag){++flagCalls;lastFlag=int(flag);return flag==1003;}
} mainGameState;
struct Registers {
 unsigned short mF8_PRINA=0xFFFF,mFA_PRINB=0xFFFF,mFC_PRIR=0xFFFF;
 unsigned short m10A_CCRNB=0xA580,mC0_WPSX0=0,mC2_WPSY0=0,mC4_WPEX0=0,mC6_WPEY0=0;
} registers;
struct {Registers* m4_pendingVdp2Regs=&registers;int m_isDirty=0;} vdp2Controls;
struct {int m0_fade0=0,m24_fade1=1;} g_fadeControls;
std::vector<int> calls;
void setVdp2LayerScroll(int layer,int x,int y){assert(!x);assert(y==(layer==3?0x100:0));calls.push_back(layer);}
void resetVdp2LayersAutoScroll(){calls.push_back(4);}
void setupVDP2StringRendering(int x,int y,int w,int h){assert(!x && y==0x22 && w==0x2C && h==0x1C);calls.push_back(5);}
void clearVdp2TextArea(){calls.push_back(6);}
void fadePalette(int* channel,int from,int to,int duration){assert(from==0xC210 && to==from && duration==1);calls.push_back(7+*channel);}
unsigned char* getVdp2Vram(int offset){assert(offset==0x71000 || offset==0x70000 || offset==0x70800);return vram+offset;}
void unpackGraphicsToVDP2(unsigned char* source,unsigned char* dest){assert(source==common+0xE3D4 && dest==vram+0x71000);calls.push_back(9);}
#include "map_entry_production.inc"
#include "map_delete_production.inc"
#include "map_ownership_production.inc"
#include "map_markers_production.inc"
int main(){
 graphicEngineStatus.m40BC_layersConfig[1].scrollX=-10;
 graphicEngineStatus.m40BC_layersConfig[1].scrollY=0x100;
 auto origin=lagiAzelSetMapViewCenter(8,6);
 assert(origin[0]==86 && origin[1]==352);
 assert(graphicEngineStatus.m40BC_layersConfig[2].scrollX==86 && graphicEngineStatus.m40BC_layersConfig[2].scrollY==352);
 assert(graphicEngineStatus.m40BC_layersConfig[1].scrollX==-10 && graphicEngineStatus.m40BC_layersConfig[1].scrollY==0x100);
 pointerWord(0xA388,0x201000);pointerWord(0x1000,0x201100);pointerWord(0x1100,0x201200);
 pointerWord(0x1104,0x201300);
 common[0x1108]=3;common[0x1109]=0xE8;
 common[0x1200]=2;common[0x1201]=3;common[0x1202]=4;common[0x1203]=2;
 pointerWord(0x1104,0x201300);pointerWord(0x1300,128*65536);
 pointerWord(0x1308,static_cast<unsigned>(-900*65536));
 common[0x130C]=3;common[0x130D]=0xEB;common[0x131E]=4;
 assert(lagiAzelDrawMapObjectMarkers(0,1,0,0,0)==1 && markerDraws==1 && markerSource==0x23F4);
 common[0x130E]=1;
 assert(lagiAzelDrawMapObjectMarkers(0,1,0,0,0)==1 && markerSource==0x23F0);
 common[0x130D]=0;
 assert(lagiAzelDrawMapObjectMarkers(0,1,0,0,0)==0);
 common[0x130D]=0xEB;common[0x130E]=2;
 assert(lagiAzelDrawMapObjectMarkers(0,1,0,0,0)==0);
 common[0x130E]=0;
 assert(lagiAzelDrawMapObjectMarkers(0,1,100,0,0)==0);
 assert(lagiAzelMapCellExplored(0,1,5,3) && mainGameState.lastFlag==1003);
 assert(!lagiAzelMapCellExplored(0,1,5,4) && mainGameState.lastFlag==1007);
 int flags=mainGameState.flagCalls;
 assert(!lagiAzelMapCellExplored(0,1,1,3) && !lagiAzelMapCellExplored(0,1,6,3));
 assert(!lagiAzelMapCellExplored(0,1,2,2) && !lagiAzelMapCellExplored(0,1,2,5));
 assert(!lagiAzelMapCellExplored(1,1,5,3) && mainGameState.flagCalls==flags);
 std::array<std::uint8_t,0x800> first{},second{};
 for(unsigned i=1;i<0x800;i+=2){first[i]=1;second[i]=2;}
 vram[0x6FFFF]=0xCD;vram[0x71000]=0xCD;
 assert(lagiAzelPresentMapPatterns(first.data(),first.size(),second.data(),second.size(),0,1));
 assert(vram[0x70000]==2 && vram[0x70001]==0x38);
 assert(vram[0x70614]==0x40 && vram[0x70615]==1);
 assert(vram[0x70800]==2 && vram[0x70801]==0x38 && vram[0x6FFFF]==0xCD && vram[0x71000]==0xCD);
 mainGameState.value=1;
 assert(lagiAzelPresentMapPatterns(first.data(),first.size(),second.data(),second.size(),0,1));
 assert(vram[0x70000]==0x70 && vram[0x70001]==1 && vram[0x70800]==0x70 && vram[0x70801]==2);
 vram[0x70000]=0xCD;
 assert(!lagiAzelPresentMapPatterns(first.data(),0x7FF,second.data(),second.size(),0,1) && vram[0x70000]==0xCD);
 for(int index=0;index<23;++index){
  fieldObject.m2C_currentFieldIndex=index;
  common[0xA650+index*2]=0;common[0xA651+index*2]=static_cast<unsigned char>(0xA0+index);
  mainGameState.value=1;assert(lagiAzelCurrentFieldMapOwned() && mainGameState.last==0xA0+index);
  mainGameState.value=0;assert(!lagiAzelCurrentFieldMapOwned());
 }
 fieldObject.m2C_currentFieldIndex=0;common[0xA650]=0xFF;common[0xA651]=0xFF;
 int count=mainGameState.calls;assert(!lagiAzelCurrentFieldMapOwned() && mainGameState.calls==count);
 fieldObject.m2C_currentFieldIndex=23;assert(!lagiAzelCurrentFieldMapOwned());
 fieldPointer=nullptr;assert(!lagiAzelCurrentFieldMapOwned());
 fieldPointer=&fieldObject;
 lagiAzelSetupMapViewWindow(-16,8);
 assert(registers.mC0_WPSX0==224 && registers.mC2_WPSY0==24 && registers.mC4_WPEX0==607 && registers.mC6_WPEY0==215);
 assert(registers.m10A_CCRNB==0xA59F && registers.mFA_PRINB==0x0705 && registers.mF8_PRINA==0x0304 && registers.mFC_PRIR==0 && vdp2Controls.m_isDirty==1);
 lagiAzelRestoreMapViewPriorities();
 assert(registers.mF8_PRINA==0x0304 && registers.mFA_PRINB==0x0700 && registers.mFC_PRIR==0 && vdp2Controls.m_isDirty==1);
 vdp2Controls.m_isDirty=0;vdp2Controls.m4_pendingVdp2Regs=nullptr;lagiAzelRestoreMapViewPriorities();assert(vdp2Controls.m_isDirty==0);
 assert(lagiAzelSetupMapEntry(false));assert(calls==std::vector<int>{9});calls.clear();
 assert(lagiAzelSetupMapEntry(true));assert((calls==std::vector<int>{0,1,2,3,4,5,6,7,8,9}));calls.clear();
 gCommonFile=nullptr;assert(!lagiAzelSetupMapEntry(true) && calls.empty());
 gCommonFile=&commonObject;commonObject.m_data=nullptr;assert(!lagiAzelSetupMapEntry(false) && calls.empty());
 puts("PASS: actual Map normal/standalone entry setup call order, native layout and null resource guards; mocked services");
}

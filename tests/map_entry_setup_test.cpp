#include <cassert>
#include <vector>
#include <cstdio>
#include <cstdint>
using u16=unsigned short;
unsigned char common[0x21084], vram[0x80000];
struct Common {unsigned char* m_data;} commonObject{common};
Common* gCommonFile=&commonObject;
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
unsigned char* getVdp2Vram(int offset){assert(offset==0x71000);return vram+offset;}
void unpackGraphicsToVDP2(unsigned char* source,unsigned char* dest){assert(source==common+0xE3D4 && dest==vram+0x71000);calls.push_back(9);}
#include "map_entry_production.inc"
#include "map_delete_production.inc"
int main(){
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

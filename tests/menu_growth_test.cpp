#include <algorithm>
#include <array>
#include <cassert>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>
using u32=std::uint32_t;using u16=std::uint16_t;using s16=std::int16_t;
struct Ptr{u32 offset;Ptr operator+(u32 n)const{return {offset+n};}};
struct Common{Ptr getSaturnPtr(u32 ea){assert(ea>=0x20701C);return {ea-0x20701C};}} common,*gCommonFile=&common;
u32 readSaturnU32(Ptr p){assert(p.offset%12==0);return p.offset/12*100;}
struct {struct{u32 m0_level=0,m20_XP=0;}gameStats;}mainGameState;
struct Command{
 u16 m0_CMDCTRL{},m4_CMDPMOD{},m6_CMDCOLR{};
 s16 mC_CMDXA{},mE_CMDYA{},m10_CMDXB{},m12_CMDYB{},m14_CMDXC{},m16_CMDYC{},m18_CMDXD{},m1A_CMDYD{};
};
struct Packet{u16 m4_bucketTypes{};Command* m6_vdp1EA{};};
struct FP{int asS32()const{return 64;}};
struct Context{Command* m0_currentVdp1WriteEA{};Packet* m20_pCurrentVdp1Packet{};u32 mC{},m1C{};};
struct {Context m14_vdp1Context[1];struct{FP m38_oneOverFarClip;}m405C;}graphicEngineStatus;
struct {u16 m4_TVSTAT{};}VDP2Regs_;
struct Text{int x,y;std::string value;};std::vector<Text> text;int x,y;
void vdp2DebugPrintSetPosition(int a,int b){x=a;y=b;}
void vdp2PrintfLargeFont(const char* format,...){char b[80];va_list args;va_start(args,format);vsnprintf(b,sizeof b,format,args);va_end(args);text.push_back({x,y,b});}
#include "../src/integration/lagi_menu_growth.inc"
Command commands[2];Packet packets[2];
void reset(u32 level,u32 xp,bool pal=false){
 mainGameState.gameStats={level,xp};text.clear();VDP2Regs_.m4_TVSTAT=pal;
 graphicEngineStatus.m14_vdp1Context[0]={commands,packets,0,0};
}
int main(){
 reset(0,0);lagiDrawMenuGrowth();assert(text.size()==3&&text[0].x==3&&text[0].y==36&&text[0].value=="LV  1");
 assert(text[1].y==44&&text[1].value=="EXP         0"&&text[2].y==46&&text[2].value=="NEXT");
 assert(!graphicEngineStatus.m14_vdp1Context[0].mC);
 reset(0,25);lagiDrawMenuGrowth();auto& c=commands[0];auto& ctx=graphicEngineStatus.m14_vdp1Context[0];
 assert(ctx.mC==1&&ctx.m1C==1&&packets[0].m6_vdp1EA==&c);
 assert(c.m0_CMDCTRL==0x1004&&c.m4_CMDPMOD==0x4C0&&c.m6_CMDCOLR==0x8256);
 assert(c.mC_CMDXA==-113&&c.m10_CMDXB==-98&&c.m14_CMDXC==-98&&c.m18_CMDXD==-113);
 assert(c.mE_CMDYA==-14&&c.m12_CMDYB==-14&&c.m16_CMDYC==-3&&c.m1A_CMDYD==-3);
 reset(0,100);lagiDrawMenuGrowth();assert(commands[0].m10_CMDXB==-50);
 reset(0,99999);lagiDrawMenuGrowth();assert(commands[0].m10_CMDXB==-50);
 reset(1,100);lagiDrawMenuGrowth();assert(!graphicEngineStatus.m14_vdp1Context[0].mC);
 reset(1,150,true);lagiDrawMenuGrowth();assert(commands[0].m10_CMDXB==-82&&commands[0].mE_CMDYA==2&&commands[0].m16_CMDYC==13);
 reset(2,50);lagiDrawMenuGrowth();assert(!graphicEngineStatus.m14_vdp1Context[0].mC);
 reset(98,123456);lagiDrawMenuGrowth();assert(text.size()==2&&text[0].value=="LV 99"&&text[1].value=="EXP    123456");
 reset(0,50);graphicEngineStatus.m14_vdp1Context[0].mC=1018;lagiDrawMenuGrowth();assert(graphicEngineStatus.m14_vdp1Context[0].mC==1018);
 puts("PASS production menu EXP/LV/NEXT text, thresholds, 63-pixel progress, native polygon/color/NTSC/PAL coordinates, cap and command bounds");
}

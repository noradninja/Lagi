#include <cstdint>
#include <string>
#include <array>
#include <cstdio>
using u8=std::uint8_t;using u32=std::uint32_t;using s16=std::int16_t;using s32=std::int32_t;
using u16=std::uint16_t;
struct s_stringStatusQuery {
 int m0_cursorX=0,m4_cursorY=0,m8_windowWidth=40,mC_windowHeight=10;
 int m10_windowX1=0,m14_windowY1=0,m18_windowX2=40,m1C_windowY2=10;
 const char* m20_string=nullptr;
 u32 m24_vdp2MemoryOffset=0,m28=0,m2C=0;
};
const char* strings[3]={"", "Tunnel", "Ruins"};
struct {int m0_numEntries=3;const char** m8_globalStrings=strings;} vdp2StringContext2;
struct {int m0=0;} vdp2StringContext;
u32 printVdp2StringTable[10]={12,13,7,8,9,10,11,13,7,14};
u32 vdp2TextMemoryOffset=0x6000;
std::array<unsigned,640> glyphs{};
unsigned centered=0;
void setVdp2VramU16(u32 address,unsigned value){if(address>=0x6000 && address<0x6500)glyphs[(address-0x6000)/2]=value;}
void printVdp2StringSub1(s_stringStatusQuery* vars){if(vars->m2C&1)++centered;}
void printVdp2StringNewLine(s_stringStatusQuery* vars){vars->m0_cursorX=vars->m10_windowX1;vars->m4_cursorY+=2;vars->m24_vdp2MemoryOffset=0x6000+(vars->m4_cursorY*64+vars->m0_cursorX)*2;}
void printVdp2String(s_stringStatusQuery* vars);
#include "global_string_production.inc"
bool check(const char* input,const char* expected){
 glyphs.fill(0);centered=0;s_stringStatusQuery query;query.m20_string=input;query.m2C=1;
 printVdp2String(&query);
 std::string actual;
 for(int i=0;i<query.m0_cursorX;++i)actual+=char(((glyphs[i]&0xFFF)-0x63)/2+0x20);
 return actual==expected && centered==1 && query.m20_string==input+std::char_traits<char>::length(input) && query.m2C==1;
}
int main(){
 if(!check("Set to %a2,","Set to Ruins,"))return 1;
 if(!check("%a1 / %a2.","Tunnel / Ruins."))return 2;
 strings[2]=nullptr;if(!check("Set to %a2,","Set to ,"))return 3;
 strings[2]="";if(!check("%a2Done","Done"))return 4;
 if(!check("%a9Done","Done"))return 5;
 strings[2]="%a1";if(!check("%a2!","Tunnel!"))return 6;
 puts("PASS: actual generated ASCII parser and substitution, destination/suffix, null/empty/bounds/nested values and no fragment recentering (host; not Vita glyph rendering)");
}

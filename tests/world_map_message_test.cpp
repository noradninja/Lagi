#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>
using s8=std::int8_t;using s16=std::int16_t;using s32=std::int32_t;using u32=std::uint32_t;
struct fixedPoint {s32 value;s32 asS32()const{return value;}};
fixedPoint MTH_Mul(s32 a,s32 b){return {s32((std::int64_t(a)*b)>>16)};}
struct sVec3_FP {s32 x,y,z;};
struct sSaturnPtr {s32 offset=0;bool isNull()const{return !offset;}sSaturnPtr operator+(int n)const{return {offset+n};}};
struct s_workArea {struct Task {bool finished=false;void markFinished(){finished=true;}} task;Task* getTask(){return &task;}};
#include "lagi/world_map_message.h"
#include "lagi/world_map_motion.h"
std::array<std::uint8_t,0x8000> bytes{};
void put(s32 p,s32 value,int n=4){for(int i=0;i<n;++i)bytes.at(p+i)=u32(value)>>((n-1-i)*8);}
s32 readSaturnS32(sSaturnPtr p){u32 value=0;for(int i=0;i<4;++i)value=value*256+bytes.at(p.offset+i);return s32(value);}
s16 readSaturnS16(sSaturnPtr p){return s16(unsigned(bytes.at(p.offset))*256+bytes.at(p.offset+1));}
sSaturnPtr readSaturnEA(sSaturnPtr p){return {readSaturnS32(p)};}
sVec3_FP readSaturnVec3(sSaturnPtr p){return {readSaturnS32(p),readSaturnS32(p+4),readSaturnS32(p+8)};}
std::string readSaturnString(sSaturnPtr p){return std::to_string(p.offset);}
struct {bool unlocked=true;bool getBit(int){return unlocked;}} mainGameState;
struct {int m4_cursorX=0,m8_cursorY=0,mC_X=2,m10_Y=22;} vdp2StringContext;
int clears=0,projections=0,creations=0,lastName=0;
std::vector<std::string> notices;
void clearVdp2TextArea(){++clears;}
void VDP2DrawString(const char* s){notices.push_back(s);}
int computeStringLength(sSaturnPtr,int limit){assert(limit==999);return 10;}
namespace lagi::worldmap {
MapLabelPlacement project_map_label(const sVec3_FP& v,s16 half){++projections;return {v.x-half,v.y,0x333};}
std::array<s_workArea,16> children;
const MapLabelPlacement* livePlacement=nullptr;
s_workArea* create_map_sprite_text(s_workArea*,sSaturnPtr name,const MapLabelPlacement* place){
 lastName=name.offset;livePlacement=place;return &children.at(creations++);
}
}
#include "message.inc"
int main(){
 using namespace lagi::worldmap;
 s_workArea owner;MapMessage s;sSaturnPtr program{1},table{0x6000};
 put(0x5C15,0);put(0x5C19,80);
 put(0x6008,100);put(0x600C,20);put(0x6010,30);put(0x6020,0x7100);
 put(0x602C,200);put(0x6030,40);put(0x6034,50);put(0x6044,0x7200);
 assert(!update_map_message(s,nullptr,program,table,2,0,false));
 assert(update_map_message(s,&owner,program,table,2,0,false));assert(s.mode==2&&clears==1&&!creations);
 assert(update_map_message(s,&owner,program,table,2,0,false));
 assert(s.mode==0&&s.child==&children[0]&&s.halfWidth==40&&s.anchor.offset==0x6008);
 assert(lastName==0x7100&&livePlacement==&s.placement&&s.placement.x==60&&s.placement.y==20);
 put(0x6008,120);update_map_message(s,&owner,program,table,2,0,false);assert(s.placement.x==80);
 update_map_message(s,&owner,program,table,2,0,true);
 assert(s.mode==1&&s.start.x==80&&s.factor==0&&s.queuedMode==-1);
 update_map_message(s,&owner,program,table,2,0,true);
 assert(s.factor==0xF5C&&s.placement.x<80&&s.placement.y>20&&s.placement.nativeZ==0x333);
 for(int n=0;n<100;++n)update_map_message(s,&owner,program,table,2,0,true);
 assert(s.factor==0x10000&&s.placement.x==-40&&s.placement.y==80);
 // Native nonzero message modes queue requests; state 0 consumes later.
 s.mode=2;request_map_message(s,3);assert(s.mode==2&&s.queuedMode==3);
 s.mode=0;update_map_message(s,&owner,program,table,2,1,false);assert(s.mode==3&&s.queuedMode==-1);
 update_map_message(s,&owner,program,table,2,1,false);assert(children[0].task.finished&&s.mode==2&&!s.child);
 update_map_message(s,&owner,program,table,2,1,false);assert(s.storedTarget==1&&lastName==0x7200);
 release_map_message(s);assert(children[1].task.finished&&!s.child);release_map_message(s);
 // Seekers' Stronghold overrides only the authored 0085 story condition.
 s.mode=3;put(0x6000,0x85,2);mainGameState.unlocked=false;
 update_map_message(s,&owner,program,table,2,0,false);update_map_message(s,&owner,program,table,2,0,false);
 assert(lastName==1+0x5C1C&&notices.back()==std::to_string(1+0x5C38));
 assert(vdp2StringContext.m4_cursorX==4&&vdp2StringContext.m8_cursorY==22);
 request_map_message(s,4);update_map_message(s,&owner,program,table,2,0,false);
 assert(!s.child&&s.mode==0&&notices.back()==std::to_string(1+0x5C50));
 request_map_message(s,5);update_map_message(s,&owner,program,table,2,0,false);
 assert(notices.back()==std::to_string(1+0x5C68));
 request_map_message(s,6);update_map_message(s,&owner,program,table,2,0,false);assert(s.mode==0&&!s.child);
 puts("PASS WORLD message lifecycle, live projection, original target centering, native text selection, queued states and child retirement; services mocked");
}

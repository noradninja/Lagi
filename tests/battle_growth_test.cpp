#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
using s32=std::int32_t;using u32=std::uint32_t;using u16=std::uint16_t;using s16=std::int16_t;using u8=std::uint8_t;using s8=std::int8_t;
struct FP{int v;int asS32()const{return v;}operator int()const{return v;}};
FP MTH_Mul(int a,int b){return {int((std::int64_t(a)*b)>>16)};}
FP FP_Div(int a,int b){assert(b);return {int((std::int64_t(a)*65536)/b)};}
struct sSaturnPtr{unsigned row,offset;sSaturnPtr operator+(unsigned n)const{return {row,offset+n};}};
struct Common{sSaturnPtr getSaturnPtr(unsigned ea){assert(ea>=0x20701C);return {(ea-0x20701C)/12,0};}}common,*gCommonFile=&common;
u32 readSaturnU32(sSaturnPtr p){assert(!p.offset);return p.row*100;}
int readSaturnS8(sSaturnPtr p){const int row[12]={0,0,0,0,10,2,3,4,4,2,6,8};assert(p.offset<12);return row[p.offset];}
unsigned readSaturnU8(sSaturnPtr p){return readSaturnS8(p);}
struct Stats{
 s16 m1C_dragonCursorY=0,m1A_dragonCursorX=0;std::array<u32,5>m24_growth{},m80_growthXP{};
 std::array<u8,8>m4_growthLevels{};u8 m0_level=0;u32 m20_XP=0;u16 mC_laserPower=0,mE_gunPower=0;
 s16 mB8_maxHP=400,mBA_maxBP=100,m12_classMaxHP=0,m16_classMaxBP=0,m10_currentHP=400,m14_currentBP=100;
};
struct {Stats gameStats;}mainGameState;
struct sBattleResultScreen{u8 m73_oldLevel=0;s8 m74=0;std::array<u16,6>m6_gains{};};
int statUpdates=0;void updateDragonStatsFromLevel(){++statUpdates;}
#include "../src/integration/lagi_battle_growth.inc"
int main(){
 auto& s=mainGameState.gameStats;s.m20_XP=200;
 battleResultScreen_updateSub5(200);for(unsigned i=1;i<5;++i)assert(s.m24_growth[i]==200&&s.m80_growthXP[i]==200);
 sBattleResultScreen r;battleResultScreen_checkLevelUp(&r);
 assert(s.m0_level==2&&r.m74==2&&r.m73_oldLevel==0);
 assert((r.m6_gains==std::array<u16,6>{24,6,6,8,6,8}));
 assert(s.mC_laserPower==12&&s.mE_gunPower==16);
 s.mB8_maxHP=9989;s.mBA_maxBP=998;lagiApplyBattleGrowth(&r);
 assert(r.m6_gains[0]==10&&r.m6_gains[1]==1&&s.m10_currentHP==410&&s.m14_currentBP==101&&statUpdates==1);
 for(unsigned i=1;i<5;++i)assert(!s.m24_growth[i]&&s.m4_growthLevels[i]==2&&s.m80_growthXP[i]==200);
 battleResultScreen_checkLevelUp(&r);assert((!r.m74&&r.m6_gains==std::array<u16,6>{}));
 s.m0_level=98;s.m20_XP=99999999;battleResultScreen_checkLevelUp(&r);assert(!r.m74&&s.m0_level==98);
 s=Stats{};s.m1C_dragonCursorY=2048;s.m1A_dragonCursorX=-2048;
 battleResultScreen_updateSub5(100);
 assert(s.m24_growth[1]==150&&s.m24_growth[2]==50&&s.m24_growth[3]==50&&s.m24_growth[4]==150);
 puts("PASS production growth: cursor weighting, native rounding, multiple levels, stat limits, cumulative proficiency, reset and level cap");
}

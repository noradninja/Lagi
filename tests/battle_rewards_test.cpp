#include <array>
#include <vector>
#include <string>
#include <cstdint>
#include <cassert>
#include <cstdio>
using s32=std::int32_t;using s16=std::int16_t;using s8=std::int8_t;using u32=std::uint32_t;
enum eItems:s16{};
enum eBattleRating:s8 {m0_narrowEscape,m1_closeCall,m2_goodFight,m3_greatFight,m4_excellent};
static std::vector<std::uint8_t> memory(385044);
struct sSaturnPtr{u32 address;bool isNull()const{return !address;}sSaturnPtr operator+(int n)const{return {address+u32(n)};}};
static unsigned readSaturnU8(sSaturnPtr p){assert(p.address>=0x6054000 && p.address-0x6054000<memory.size());return memory[p.address-0x6054000];}
static unsigned readSaturnU16(sSaturnPtr p){return (readSaturnU8(p)<<8)|readSaturnU8(p+1);}
static s32 readSaturnS32(sSaturnPtr p){return s32((readSaturnU16(p)<<16)|readSaturnU16(p+2));}
static sSaturnPtr readSaturnEA(sSaturnPtr p){return {u32(readSaturnS32(p))};}
static void word(u32 ea,unsigned v){memory[ea-0x6054000]=std::uint8_t(v>>8);memory[ea+1-0x6054000]=std::uint8_t(v);}
static void longword(u32 ea,u32 v){word(ea,v>>16);word(ea+2,v);}
struct Generic{sSaturnPtr getSaturnPtr(u32 a){return {a};}}generic;
static auto* g_BTL_GenericData=&generic;
struct Engine{std::array<s16,2> m480{-1,-1};}engine;
struct Overlay{Engine* m4_battleEngine=&engine;}overlay;
struct Manager{int m4_previousBattleOverlayId=0,m8_previousSubBattleId=0,m2_currentBattleOverlayId=9,m6_subBattleId=9;Overlay* m10_battleOverlay=&overlay;}manager;
static auto* gBattleManager=&manager;
struct State{
 std::array<s8,77> consumables{};std::array<bool,5040> bits{};
 bool getBit(u32 n){return bits.at(n);}void setBit(u32 n){bits.at(n)=true;}void clearBit(u32 n){bits.at(n)=false;}
#include "inventory.inc"
}mainGameState;
static unsigned rngCalls=0,roll=0,labels=0,icons=0,names=0;
static unsigned randomNumber(){++rngCalls;return roll;}
static int performModulo2(int divisor,unsigned value){return value%divisor;}
static void battleResultScreenProgressiveText_printString(sSaturnPtr p){assert(p.address==0x60ABA00);++labels;}
static void displayObjectIcon(int mode,int x,int y,int icon){assert(!mode && x==10 && y==14 && icon==2);++icons;}
static int getObjectIcon(eItems){return 2;}
static struct{std::string m4_name="Bonus";}object;
static auto* getObjectListEntry(eItems){return &object;}
static struct{int m10_palette=123,m14_oldPalette=0;}vdp2PrintStatus;
static void vdp2DebugPrintSetPosition(int x,int y){assert(x==12 && y==14);}
static void vdp2PrintfLargeFont(const char* format,const char* name){assert(std::string(format)=="%s" && std::string(name)=="Bonus" && vdp2PrintStatus.m10_palette==0xc000);++names;}
template<class... T>static void PDS_Log(const char*,T...){}
#include "../src/integration/lagi_battle_rewards.inc"
#include "native_reward_rows.inc"
int main(){
 longword(0x60AAFD8,0x60A9824);
 for(unsigned i=0;i<sizeof(nativeRows);++i)memory[0x60A9824-0x6054000+i]=nativeRows[i];
 longword(0x60ABAB0,10);longword(0x60ABAB4,14);longword(0x60ABAB8,12);
 for(int sub=0;sub<8;++sub)for(int rank=0;rank<5;++rank)for(roll=0;roll<100;++roll){
  manager.m8_previousSubBattleId=sub;engine.m480={-1,-1};mainGameState={};
  const auto calls=rngCalls;const auto prints=labels;
  battleResultScreen_printItemReceived(static_cast<eBattleRating>(rank));
  const int expected=rank==3 && roll<30?0x1c:rank==4 && roll<30?0x1d:-1;
  assert(engine.m480[0]==expected && engine.m480[1]==-1 && rngCalls==calls+1);
  assert(labels==prints+(expected>=0));assert(labels==icons && icons==names && vdp2PrintStatus.m10_palette==123);
  if(expected>=0)assert(mainGameState.getItemCount(static_cast<eItems>(expected))==1);
 }
 // No table consumes no RNG, but a previously recorded reward must still award.
 longword(0x60AAFD8,0);engine.m480={5,-1};mainGameState={};mainGameState.consumables[5]=98;
 const auto calls=rngCalls;battleResultScreen_printItemReceived(m0_narrowEscape);
 assert(rngCalls==calls && mainGameState.consumables[5]==99);
 battleResultScreen_printItemReceived(m0_narrowEscape);assert(mainGameState.consumables[5]==99);
 engine.m480[0]=0x4d;battleResultScreen_printItemReceived(m0_narrowEscape);
 assert(mainGameState.getItemCount(static_cast<eItems>(0x4d))==1);
 battleResultScreen_printItemReceived(m0_narrowEscape);assert(mainGameState.getItemCount(static_cast<eItems>(0x4d))==1);
 // Asymmetric authored thresholds and item choices, including exact equality.
 longword(0x60AAFD8,0x60A9824);manager.m8_previousSubBattleId=0;
 word(0x60A9824,5);word(0x60A9826,6);memory[0x60A9828-0x6054000]=17;memory[0x60A9829-0x6054000]=83;
 for(int rank=3;rank<=4;++rank)for(roll=0;roll<100;++roll){engine.m480={-1,-1};
  battleResultScreen_printItemReceived(static_cast<eBattleRating>(rank));
  assert(engine.m480[0]==(roll<unsigned(rank==3?17:83)?rank==3?5:6:-1));
 }
 memory[0x60A9828-0x6054000]=0;memory[0x60A9829-0x6054000]=100;
 roll=99;engine.m480={-1,-1};battleResultScreen_printItemReceived(m4_excellent);assert(engine.m480[0]==6);
 roll=0;engine.m480={-1,-1};battleResultScreen_printItemReceived(m3_greatFight);assert(engine.m480[0]==-1);
 word(0x60A9826,0xffff);engine.m480={-1,-1};
 const auto before=labels;battleResultScreen_printItemReceived(m4_excellent);assert(engine.m480[0]==-1 && labels==before);
 std::puts("PASS production bonus rewards: all native first-eight A3 rows/ranks/rolls, pre-remap IDs, RNG consumption, threshold equality, actual inventory methods, cap/flag items and native-layout announcement; no save/Vita proof");
}

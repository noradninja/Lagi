#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>
using s16=std::int16_t;using s32=std::int32_t;using u32=std::uint32_t;using s8=std::int8_t;
enum eItems:s16{};
static std::array<unsigned char,4096> memory{};
struct Ptr {unsigned offset;Ptr operator+(unsigned n)const{return {offset+n};}};
static unsigned readSaturnU8(Ptr p){return memory.at(p.offset);}
static unsigned readSaturnU16(Ptr p){return readSaturnU8(p)*256+readSaturnU8(p+1);}
static s32 readSaturnS32(Ptr p){return s32(readSaturnU16(p)*65536+readSaturnU16(p+2));}
static Ptr readSaturnEA(Ptr p){return {unsigned(readSaturnS32(p))};}
static void word(unsigned p,unsigned v){memory.at(p)=v>>8;memory.at(p+1)=v;}
static void number(unsigned p,unsigned v){word(p,v>>16);word(p+2,v);}
struct Generic {Ptr getSaturnPtr(unsigned ea){
 if(ea==0x60AC094)return {0};
 if(ea==0x60ABA70)return {2000};
 assert(ea>=0x60ABAD0&&ea<0x60ABB10);return {2100+ea-0x60ABAD0};
}}generic,*g_BTL_GenericData=&generic;
struct State {
 struct {std::array<unsigned char,8> m4_growthLevels{};}gameStats;
 std::array<s8,77> consumables{};std::array<bool,5040>bits{};
 bool getBit(u32 n){return bits.at(n);}void setBit(u32 n){bits.at(n)=true;}void clearBit(u32 n){bits.at(n)=false;}
#include "inventory.inc"
}mainGameState;
struct sBattleResultScreen {
 std::array<std::array<s16,8>,6>m12_learnedBerserks{};
 std::array<unsigned char,6>m7A_learnedCounts{};
};
static std::vector<int> unlocked,displayed;
template<class... T>void PDS_Log(const char*,int,int item,T...){unlocked.push_back(item);}
static unsigned labels=0,names=0;
void battleResultScreenProgressiveText_printString(Ptr p){assert(p.offset==2000);++labels;}
int getObjectIcon(eItems item){return int(item);}
void displayObjectIcon(int mode,int x,int y,int item){assert(!mode&&x==int(names)+10&&y==20);displayed.push_back(item);}
struct Object{std::string m4_name="Ability";}object;
Object* getObjectListEntry(eItems){return &object;}
struct {int m10_palette=123,m14_oldPalette=0;}vdp2PrintStatus;
void vdp2DebugPrintSetPosition(int x,int y){assert(x==int(names)+12&&y==20);}
void vdp2PrintfLargeFont(const char*,const char*){assert(vdp2PrintStatus.m10_palette==0xc000);++names;}
#include "../src/integration/lagi_battle_berserk_rewards.inc"
int main(){
 for(unsigned c=0;c<4;++c){number(c*4,100+c*80);for(unsigned i=0;i<5;++i){
  auto p=100+c*80+i*8;memory[p]=255; // byte zero is not an eligibility input
  for(unsigned k=1;k<5;++k)memory[p+k]=3;
  word(p+6,0x4d+c*5+i);
 }}
 for(unsigned i=0;i<4;++i){number(2100+i*16,10+i);number(2104+i*16,20);number(2108+i*16,12);}
 sBattleResultScreen r;
 lagiUnlockBattleBerserks(&r);assert(unlocked.empty());
 for(unsigned k=1;k<5;++k)mainGameState.gameStats.m4_growthLevels[k]=2;
 mainGameState.setItemCount(static_cast<eItems>(0x4d),1);
 lagiUnlockBattleBerserks(&r);assert(unlocked.size()==19&&unlocked.front()==0x5c&&unlocked.back()==0x51);
 assert(r.m7A_learnedCounts[0]==4&&r.m7A_learnedCounts[3]==5);
 for(int id=0x4d;id<0x61;++id)assert(mainGameState.getItemCount(static_cast<eItems>(id))==1);
 lagiPrintLearnedBattleBerserks(&r);assert(names==4&&labels==1&&vdp2PrintStatus.m10_palette==123);
 assert((displayed==std::vector<int>{0x4e,0x4f,0x50,0x51}));
 lagiUnlockBattleBerserks(&r);for(auto n:r.m7A_learnedCounts)assert(!n);
 lagiPrintLearnedBattleBerserks(&r);assert(labels==1&&unlocked.size()==19);
 // A missing requirement in any individual proficiency must block learning.
 for(unsigned k=1;k<5;++k){mainGameState={};for(unsigned j=1;j<5;++j)mainGameState.gameStats.m4_growthLevels[j]=2;
  mainGameState.gameStats.m4_growthLevels[k]=1;lagiUnlockBattleBerserks(&r);
  for(auto n:r.m7A_learnedCounts)assert(!n);
 }
 puts("PASS production Berserk rewards: four requirements, equality, ownership, sentinel, ordering, four-name limit and palette restore");
}

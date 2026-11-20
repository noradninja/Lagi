#include <array>
#include <bit>
#include <bitset>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include "lagi/save_growth_migration.h"
using u8=std::uint8_t;using s8=std::int8_t;using u16=std::uint16_t;using s16=std::int16_t;using u32=std::uint32_t;using s32=std::int32_t;
enum eItems:s16{};enum e_dragonLevel:unsigned char{};enum e_dragonArchetype:unsigned char{};
static std::vector<int>m_bitFieldMap;
u32 readPackedBits(u8*,u32,u32){return 0;} // description only; not serialized data
#include "reward_save_state.inc"
s_mainGameState mainGameState{};
sSaveGameStatus gSaveGameStatus{};
std::array<u8,0x104>battleResults{};
struct {struct {std::array<std::array<u16,16>,3>mD8_buttonConfig{};}m4514;}graphicEngineStatus;
int intDivide(int divisor,int value){return value/divisor;}
int performModulo(int divisor,int value){return value%divisor;}
static FILE* disk;
int lagiAzelSaveWrite(int,const char*,void* data,std::size_t bytes){
 std::rewind(disk);return std::fwrite(data,1,bytes,disk)==bytes&&std::fflush(disk)==0?0:-1;
}
int lagiAzelSaveReadGrowth(int,const char*,void* data,std::size_t bytes,std::size_t at){
 std::rewind(disk);auto n=std::fread(data,1,bytes,disk);
 return lagi::save::expand_growth(static_cast<u8*>(data),n,bytes,at)?0:-2;
}
#include "reward_save_functions.inc"
int main(){
 static_assert(sizeof(sSaveGameStatus)==12);
 static_assert(sizeof(s_gameStats)-offsetof(s_gameStats,m24_growth)==48);
 disk=std::tmpfile();assert(disk);
 const std::string slot="test";
 mainGameState.setItemCount(static_cast<eItems>(0x1d),17);
 mainGameState.setItemCount(static_cast<eItems>(0x4d),1);
 mainGameState.setItemCount(static_cast<eItems>(0x60),1);
 // Disc1 COMMON20A650 field->item IDs, including the shared 06/07 and
 // 0F/12 maps. Keep multiple acquired maps through native save and migration.
 constexpr std::array<int,10> maps={0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xBA,0xBB,0xBC,0xBD};
 for(int item:maps)mainGameState.setItemCount(static_cast<eItems>(item),1);
 for(unsigned flag=0x95A;flag<0x10D8;++flag)
  if((flag-0x95A)%5==0 || flag==0x10D7)mainGameState.setBit566(flag);
 const auto checkMaps=[&]{
  for(int item:maps)assert(mainGameState.getItemCount(static_cast<eItems>(item))==1);
  for(unsigned flag=0x95A;flag<0x10D8;++flag)
   assert(mainGameState.getBit566(flag)==((flag-0x95A)%5==0 || flag==0x10D7));
 };
 auto& s=mainGameState.gameStats;s.m0_level=7;s.m20_XP=12345;s.m38_dyne=8765;
 for(unsigned i=1;i<5;++i){s.m24_growth[i]=i*7;s.m80_growthXP[i]=i*111;s.m4_growthLevels[i]=i+2;}
 battleResults[8]=255;battleResults[9]=4;
 graphicEngineStatus.m4514.mD8_buttonConfig[2][15]=0x1234;
 assert(saveData(0,&slot)==0);
 mainGameState={};battleResults.fill(0);graphicEngineStatus={};
 assert(readSave(0,slot)==0);
 checkMaps();
 assert(mainGameState.getItemCount(static_cast<eItems>(0x1d))==17);
 assert(mainGameState.getItemCount(static_cast<eItems>(0x4d))==1);
 assert(mainGameState.getItemCount(static_cast<eItems>(0x60))==1);
 assert(s.m0_level==7&&s.m20_XP==12345&&s.m38_dyne==8765);
 for(unsigned i=1;i<5;++i)assert(s.m24_growth[i]==i*7&&s.m80_growthXP[i]==i*111&&s.m4_growthLevels[i]==i+2);
 assert(battleResults[8]==255&&battleResults[9]==4);
 assert(graphicEngineStatus.m4514.mD8_buttonConfig[2][15]==0x1234);
 // Corrupt data must not replace live progression or ownership.
 std::fseek(disk,100,SEEK_SET);std::fputc(255,disk);std::fflush(disk);
 assert(readSave(0,slot)!=0&&s.m20_XP==12345&&mainGameState.getItemCount(static_cast<eItems>(0x4d))==1);
 checkMaps();
 std::fclose(disk);
 // Model an older save by removing only the appended 48-byte extension.
 disk=std::tmpfile();assert(disk);assert(saveData(0,&slot)==0);
 std::array<u8,sizeof(sSaveDataRaw)> current{};std::rewind(disk);
 assert(std::fread(current.data(),1,current.size(),disk)==current.size());
 constexpr auto at=offsetof(sSaveDataRaw,mC_mainGameState)+offsetof(s_mainGameState,gameStats)+offsetof(s_gameStats,m24_growth);
 std::vector<u8> legacy;legacy.insert(legacy.end(),current.begin(),current.begin()+at);
 legacy.insert(legacy.end(),current.begin()+at+48,current.end());
 const u32 oldVersion=0x10000;std::memcpy(legacy.data()+4,&oldVersion,4);
 const auto sum=lagi::save::checksum(legacy.data(),legacy.size());std::memcpy(legacy.data(),&sum,4);
 std::fclose(disk);disk=std::tmpfile();assert(disk);
 assert(std::fwrite(legacy.data(),1,legacy.size(),disk)==legacy.size());std::fflush(disk);
 mainGameState={};battleResults.fill(0);graphicEngineStatus={};
 assert(readSave(0,slot)==0&&s.m20_XP==12345);
 checkMaps();
 assert(mainGameState.getItemCount(static_cast<eItems>(0x1d))==17);
 assert(mainGameState.getItemCount(static_cast<eItems>(0x4d))==1);
 assert(mainGameState.getItemCount(static_cast<eItems>(0x60))==1);
 for(unsigned i=0;i<5;++i)assert(!s.m24_growth[i]&&!s.m80_growthXP[i]&&!s.m4_growthLevels[i]);
 assert(battleResults[8]==255&&battleResults[9]==4&&graphicEngineStatus.m4514.mD8_buttonConfig[2][15]==0x1234);
 std::fclose(disk);
 puts("PASS generated save/read disk roundtrip: multiple field maps, complete sparse exploration range, legacy migration, consumables, Berserk flags, XP, growth, enemy records, controls and corrupt-file rejection; host I/O shim, not Vita validation");
}

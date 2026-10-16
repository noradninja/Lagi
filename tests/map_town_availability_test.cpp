#include "lagi/azel_map_menu.h"
#include <cassert>
#include <cstdio>
struct GameState {int scene=2;bool story=false;
 int readPackedBits(int first,int count){assert(first==0 && count==2);return scene;}
 bool getBit(int bit){assert(bit==193);return story;}}mainGameState;
struct {std::int8_t m5C=0;}npcData0;
struct Common {unsigned m_base=0x200000;std::uint8_t* m_data;std::size_t m_dataSize;};Common* gCommonFile=nullptr;
#include "map_town_availability.inc"
int main(){
 LagiAzelMapAvailability out;
 assert(lagiAzelResolveTownMapAvailability(out));
 assert(out.slots[0]==0 && out.slots[1]==-1 && out.worldCover && out.selected==0);
 mainGameState.story=true;assert(!lagiAzelResolveTownMapAvailability(out));
 std::vector<std::uint8_t> bytes(134788);Common common{0x200000,bytes.data(),bytes.size()};gCommonFile=&common;
 unsigned cases=0;
 for(int current=-128;current<128;++current)for(int destination=-128;destination<128;++destination){
  npcData0.m5C=static_cast<std::int8_t>(current);bytes[0x166E4]=static_cast<std::uint8_t>(destination);
  assert(lagiAzelResolveTownMapAvailability(out));
  assert(out.slots[0]==0 && out.slots[1]==int(current!=destination));
  assert(out.selected==int(current!=destination) && !out.worldCover);++cases;
 }
 for(int scene:{0,1,3}){mainGameState.scene=scene;assert(!lagiAzelResolveTownMapAvailability(out));}
 mainGameState.scene=2;common.m_dataSize=0x166E4;assert(!lagiAzelResolveTownMapAvailability(out));
 common.m_dataSize=bytes.size();common.m_base=0x300000;assert(!lagiAzelResolveTownMapAvailability(out));
 common.m_base=0x200000;common.m_data=nullptr;assert(!lagiAzelResolveTownMapAvailability(out));
 mainGameState.story=false;assert(lagiAzelResolveTownMapAvailability(out) && out.worldCover && out.slots[1]==-1);
 printf("PASS: production town Map native scene/story/current-town/table binding, %u signed comparisons, absent-choice short circuit and malformed COMMON guards; parent/device untested\n",cases);
}

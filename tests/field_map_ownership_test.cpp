#include <cstdint>
#include <cassert>
#include <cstring>
#include <cstdio>
using u32=std::uint32_t;using s8=std::int8_t;using eItems=int;
struct State {
 std::uint8_t bitField[630]{};
 std::int8_t consumables[0x4D]{};
#include "item_ownership_production.inc"
} mainGameState;
struct Target {s8 m18_diableFlags=0,m19=0;};
struct s_itemBoxType1 {int m88_poseIdx=0xB2;Target m8_LCSTarget;s8 m20=0x55,m21=0x33;};
unsigned registrations=0;
void updateLCSTarget(Target* target){if(!(target->m18_diableFlags&1))++registrations;}
void mapOwnership(s_itemBoxType1* pThis) {
#include "map_ownership_production.inc"
}
int main(){
 s_itemBoxType1 map;
 mapOwnership(&map);
 if(registrations!=1 || mainGameState.getItemCount(0xB2))return 1;
 mainGameState.addItemCount(0xB2,1); // existing native laser reward path
 if(!mainGameState.getItemCount(0xB2) || !mainGameState.getBit(0x1A5))return 2;
 for(unsigned frame=0;frame<100;++frame)mapOwnership(&map);
 if(registrations!=1 || !(map.m8_LCSTarget.m18_diableFlags&1) || map.m20!=0x55 || map.m21!=0x33)return 3;
 s_itemBoxType1 reopened;
 mapOwnership(&reopened);
 if(registrations!=1 || !(reopened.m8_LCSTarget.m18_diableFlags&1))return 4;
 mainGameState.addItemCount(0xB2,1);
 if(mainGameState.getItemCount(0xB2)!=1)return 5;
 puts("PASS: actual native item ownership methods and generated map target tail; flag 1A5, repeat targeting and recreated pickup (host; not device save persistence or Map page)");
}

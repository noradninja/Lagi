#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
using s32=std::int32_t;
struct Field {int m2C_currentFieldIndex=0;} field;
auto* fieldTaskPtr=&field;
struct GameState {
 std::array<std::uint8_t,32> bits{};
 int calls=0,lastValue=-1;
 void setPackedBits(unsigned offset,unsigned count,unsigned value){
  assert(offset==0x87&&count==6);++calls;lastValue=int(value);
  for(unsigned i=0;i<count;++i){
   const unsigned bit=offset+i,mask=0x80>>(bit%8);
   bits[bit/8]=std::uint8_t((bits[bit/8]&~mask)|(((value>>(count-1-i))&1)?mask:0));
  }
 }
}mainGameState;
#include "production.inc"
int main(){
 for(int current : {0,3,6,17})
  for(int target : {0,2,3,6,17,18})
   for(unsigned pattern=0;pattern<256;++pattern){
    field.m2C_currentFieldIndex=current;
    for(unsigned i=0;i<32;++i)mainGameState.bits[i]=std::uint8_t(pattern+i*7);
    const auto before=mainGameState.bits;
    mainGameState.calls=0;mainGameState.lastValue=-1;
    exitCutsceneTaskUpdateSub0Sub1Sub0Sub0(target);
    const int status=target==3?0x56:target==6?0x63:target==17?0x6D:-1;
    auto expected=before;
    if(target!=current&&status>=0){
     const unsigned index=unsigned(status-0x50);
     assert(mainGameState.calls==1&&mainGameState.lastValue==int(index));
     expected[16]=std::uint8_t((expected[16]&0xFE)|(index>>5));
     expected[17]=std::uint8_t((expected[17]&7)|((index&31)<<3));
    }else assert(mainGameState.calls==0);
    assert(mainGameState.bits==expected&&field.m2C_currentFieldIndex==current);
   }
 puts("PASS generated native field-to-WORLD route conversion: field3->6, field6->19, field17->29; same-field/foreign-field guards and unrelated packed bits preserved; device re-entry unverified");
}

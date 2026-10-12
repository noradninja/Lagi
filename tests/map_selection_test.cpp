#include "lagi/azel_map_menu.h"
#include <cassert>
#include <cstdio>
int main(){
 using R=LagiAzelMapSelectionResult;
 std::int32_t selected=0;std::array<std::int32_t,2> slots{1,0};
 auto r=lagiAzelMapSelect(selected,slots,7);assert(r.action==R::Cancel && r.sound==1 && selected==0);
 r=lagiAzelMapSelect(selected,slots,2);assert(r.action==R::EnterMode1 && r.sound==0);
 r=lagiAzelMapSelect(selected,slots,0x10);assert(selected==1 && r.action==R::Moved && r.oldHighlight==0 && r.newHighlight==-1);
 r=lagiAzelMapSelect(selected,slots,4);assert(r.action==R::Rejected && r.sound==5);
 slots={-1,1};r=lagiAzelMapSelect(selected,slots,0x20);assert(selected==1 && r.oldHighlight==1 && r.newHighlight==1);
 r=lagiAzelMapSelect(selected,slots,2);assert(r.action==R::EnterMode2);
 for(int a=-1;a<=1;++a)for(int b=-1;b<=1;++b)
 for(int cursor=-1;cursor<=1;++cursor)for(int input=0;input<64;++input){
  slots={a,b};selected=cursor;r=lagiAzelMapSelect(selected,slots,input);
  assert(selected>=-1 && selected<=1);
  if(input&1) assert(r.action==R::Cancel);
  if(r.action==R::Moved) assert(slots[selected]!=-1);
 }
 puts("PASS: native Map selection cancel/confirm/reject/wrap/highlights and 1728 bounded input cases; task/display untested");
}

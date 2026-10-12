#include "lagi/azel_map_menu.h"
#include <cassert>
#include <cstdio>
int main(){
 unsigned tested=0;
 for(int offset:{-16,0,16})for(int sx:{-20,0,20})for(int sy:{-20,0,20})
 for(int x=95;x<=289;++x)for(int y=31-offset;y<=225-offset;++y){
  std::array<std::int16_t,2> out{123,456};
  bool clipped=lagiAzelMapMarkerScreenPosition(x+sx,y+sy,sx,sy,offset,out);
  bool expected=x>=96 && x<=288 && y>=32-offset && y<=224-offset;
  assert(clipped==expected);
  if(expected)assert(out[0]==x && out[1]==y);
  else assert(out[0]==123 && out[1]==456);
  ++tested;
 }
 std::printf("PASS: %u native map marker clip/scroll/offset cases; marker drawing untested\n",tested);
}

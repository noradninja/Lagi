#include <cassert>
#include <cstdio>
struct Parent {int m8B_LCSType,mEA_state;};
#include "map_particle_gates.inc"
int main(){
 for(int type=1;type<=2;++type)
  for(int state=0;state<4;++state){
   Parent parent{type,state};
   const bool blocked=type==1 ? state!=0 : state==0;
   assert(updateBlocked(&parent)==blocked);
   assert(drawBlocked(&parent)==blocked);
  }
 puts("PASS: actual generated map/station particle Update and Draw gates across collection states; not device appearance");
}

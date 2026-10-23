#include <array>
#include <vector>
#include <cassert>
#include <cstdio>
static std::vector<std::array<int,4>> requests;
void enqueuePlaySoundEffect(int sound,int command,int volume,int arg){requests.push_back({sound,command,volume,arg});}
#include "sound_stop.inc"
int main(){
 for(unsigned entry=0;entry<4;++entry){soundFunc(100);soundFunc(101);}
 assert(requests.size()==8);
 for(unsigned i=0;i<requests.size();++i){
  assert((requests[i]==std::array<int,4>{i%2?101:100,2,0,0}));
 }
 soundFunc(67);assert((requests.back()==std::array<int,4>{67,2,0,0}));
 puts("PASS generated native stop wrapper for repeated tunnel entries and arbitrary sound IDs; no bank/scene filter, not device listening proof");
}

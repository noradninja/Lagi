#include "lagi/azel_map_menu.h"
#include <vector>
#include <string>
#include <cstdio>
#include <algorithm>
std::uint32_t graphicBytes=0,patternBytes=0;
std::array<std::uint8_t,0x80000> vram;
std::vector<std::string> loads;
std::vector<std::uint8_t*> destinations;
int failAt=0;
std::uint32_t getFileSize(const char* file){return std::string(file)=="map.SCB"?graphicBytes:patternBytes;}
std::uint8_t* getVdp2Vram(std::uint32_t offset){return vram.data()+offset;}
int loadFile2(const char* file,std::uint8_t* destination,std::uint16_t patch){
 if(patch)return -1;
 loads.push_back(file);destinations.push_back(destination);
 if(int(loads.size())==failAt)return -1;
 std::fill(destination,destination+getFileSize(file),0xAB);
 return 0;
}
#include "map_loader_production.inc"
int main(){
 LagiAzelMapDescriptor descriptor;descriptor.mapFile="map.SCB";descriptor.patternFile="map.PNB";
 std::array<std::uint8_t,0x1000> patterns;
 unsigned cases=0;
 for(unsigned graphics:{0x3FFFu,0x4000u,0x4001u,0x17000u})
 for(unsigned pnb:{1u,0x7FFu,0x800u,0x801u,0x1000u}){
  graphicBytes=graphics;patternBytes=pnb;patterns.fill(0xCD);loads.clear();destinations.clear();
  std::uint32_t offset=0;
  if(!lagiAzelLoadFieldMapResources(descriptor,patterns.data(),patterns.size(),offset))return 1;
  const auto expected=graphics>0x4000?0x69000u:0x6C000u;
  if(offset!=expected || loads!=std::vector<std::string>{"map.SCB","map.PNB"} || destinations[0]!=vram.data()+expected || destinations[1]!=patterns.data())return 2;
  for(unsigned i=0;i<patterns.size();++i){
   const unsigned expectedByte=i<pnb?0xAB:((pnb<=0x800 && i>=0x800)?((i&1)?0x38:2):0xCD);
   if(patterns[i]!=expectedByte)return 3;
  }
  ++cases;
 }
 loads.clear();graphicBytes=0;
 std::uint32_t offset=77;
 if(lagiAzelLoadFieldMapResources(descriptor,patterns.data(),patterns.size(),offset) || offset || !loads.empty())return 4;
 graphicBytes=0x17001;
 if(lagiAzelLoadFieldMapResources(descriptor,patterns.data(),patterns.size(),offset) || !loads.empty())return 5;
 graphicBytes=0x4000;patternBytes=1;
 if(lagiAzelLoadFieldMapResources(descriptor,patterns.data(),0xFFF,offset) || !loads.empty())return 6;
 for(int failure:{1,2}){
  loads.clear();failAt=failure;
  if(lagiAzelLoadFieldMapResources(descriptor,patterns.data(),patterns.size(),offset) || offset || int(loads.size())!=failure)return 7;
 }
 std::printf("PASS: %u actual map loader threshold/fill cases, bounds and both native load failures (host mocks, not renderer/task execution)\n",cases);
}

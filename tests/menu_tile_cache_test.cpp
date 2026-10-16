#include "lagi/lagi_menu_tile_cache.h"
#include <algorithm>
#include <cassert>
#include <cstdio>
constexpr std::size_t kVdp2TextSnapshotBytes=0x80000,kVdp2CramSnapshotBytes=0x1000;
unsigned char g_vdp2TextVram[kVdp2TextSnapshotBytes],g_vdp2Cram[kVdp2CramSnapshotBytes];
std::uint16_t readVdp2Be16(const unsigned char* p,std::size_t i){return (p[i]<<8)|p[i+1];}
#include "menu_tile_rgb.inc"
#include "menu_tile_character.inc"
#include "menu_tile_cache_production.inc"
int main(){
 std::uint32_t rng=1234567;auto random=[&](){rng=rng*1664525u+1013904223u;return rng;};
 for(auto& byte:g_vdp2TextVram)byte=random()>>24;
 for(auto& byte:g_vdp2Cram)byte=random()>>24;
 unsigned checks=0;
 for(int frame=0;frame<3;++frame){
  g_menuTileCache.beginFrame();
  for(bool save:{false,true})for(unsigned p=0;p<128;++p){
   const auto pattern=std::uint16_t(p*509u);
   for(int y=0;y<16;++y)for(int x=0;x<16;++x){
    const auto raw=save?decodeSaveMenuCharacter(pattern,x,y):decodeVdp2Chsz1Pixel(pattern,x,y,6,12);
    assert(cachedMenuCharacter(pattern,x,y,save)==raw);++checks;
   }
  }
  // Simulate next published image replacing character/palette bytes.
  for(auto& byte:g_vdp2TextVram)byte=random()>>24;
  for(auto& byte:g_vdp2Cram)byte=random()>>24;
 }
 for(bool save:{false,true}){
  const unsigned map=save?0x20000:0x71000;
  g_menuTileCache.beginFrame();
  for(int scroll:{-1025,-1,0,7,16,511,1024})for(int y=0;y<224;y+=7)for(int x=0;x<352;x+=7){
   const unsigned mx=(x+scroll)&511,my=(y+scroll)&511;
   const auto p=readVdp2Be16(g_vdp2TextVram,map+((my>>4)*32+(mx>>4))*2);
   const auto raw=save?decodeSaveMenuCharacter(p,mx&15,my&15):(p?decodeVdp2Chsz1Pixel(p,mx&15,my&15,6,12):0);
   assert((save?decodeSaveMenuNbgPixel(map,x,y,scroll,scroll):decodeStatusMenuNbgPixel(map,x,y,scroll,scroll))==raw);++checks;
  }
 }
 // Exact dependency invalidation, immutable frame and forced hash collisions.
 lagi::MenuTileCache<1> small;std::uint8_t character=3,palette=7;unsigned decodes=0,validations=0;
 auto sources=[&](){++validations;return lagi::MenuTileSources{&character,1,&palette,1};};
 auto decode=[&](unsigned x,unsigned y){++decodes;return character+palette+x+y;};
 small.beginFrame();assert(small.tile(1,sources,decode)[0]==10);
 for(int i=0;i<1000;++i)assert(small.tile(1,sources,decode)[0]==10);
 assert(decodes==256 && validations==1);
 small.beginFrame();assert(small.tile(1,sources,decode)[0]==10 && decodes==256 && validations==2);
 character=4;small.beginFrame();assert(small.tile(1,sources,decode)[0]==11 && decodes==512);
 palette=8;small.beginFrame();assert(small.tile(1,sources,decode)[0]==12 && decodes==768);
 assert(small.tile(2,sources,decode)[0]==12 && decodes==1024);
 assert(small.tile(1,sources,decode)[0]==12 && decodes==1280);
 printf("PASS: %u cached/uncached native status/save pixels, flip/palette/scroll/address bounds, source mutation and collision cases; repeated samples validate once per frame and reuse decoded pixels (host, not Vita timing)\n",checks);
}

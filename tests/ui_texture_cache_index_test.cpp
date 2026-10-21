#include "lagi/ui_texture_cache_index.h"
#include <cassert>
#include <cstdio>
#include <vector>
int main(){
 lagi::UiTextureCacheIndex cache;std::vector<unsigned> entries;
 for(unsigned i=0;i<4096;++i){
  const auto key=lagi::ui_texture_key(i,0xabcd,i^0x1234,0x631);
  std::size_t index=99999;assert(!cache.find(key,index)&&index==99999);
  entries.push_back(i);cache.insert(key,entries.size()-1);
 }
 for(unsigned i=0;i<4096;++i){std::size_t index;
  assert(cache.find(lagi::ui_texture_key(i,0xabcd,i^0x1234,0x631),index));
  assert(entries[index]==i);
 }
 const auto key=lagi::ui_texture_key(1,2,3,4);
 assert(key==0x0004000300020001ULL);
 for(unsigned field=0;field<4;++field){
  std::uint16_t v[4]={1,2,3,4};v[field]^=0x8000;
  assert(lagi::ui_texture_key(v[0],v[1],v[2],v[3])!=key);
 }
 std::size_t index;cache.insert(key,42);assert(cache.find(key,index)&&index==42);
 const float full[4][2]={{-1,-1},{1,-1},{1,1},{-1,1}};
 const float offscreen[4][2]={{2,2},{3,2},{3,3},{2,3}};
 const float half[4][2]={{-2,-1},{0,-1},{0,1},{-2,1}};
 assert(lagi::sprite_bbox_pixels(full,960,544)==960u*544u);
 assert(lagi::sprite_bbox_pixels(offscreen,960,544)==0);
 assert(lagi::sprite_bbox_pixels(half,960,544)==480u*544u);
 lagi::UiSpriteProfile profile;profile.draws=20;profile.bboxPixels=1000;profile={};
 assert(!profile.draws&&!profile.bboxPixels&&!profile.slotSkips);
 puts("PASS: exact sprite descriptor index, reallocation-safe offsets, clipped coverage and reset; not GPU validation");
}

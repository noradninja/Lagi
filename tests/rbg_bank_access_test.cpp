#include "lagi/rbg_decoded_page.h"
#include <cassert>
#include <cstdio>
#include <vector>
#include <algorithm>
int main(){
 unsigned checks=0;
 for(unsigned ctl=0;ctl<1024;++ctl)for(unsigned bank=0;bank<4;++bank){
  unsigned effective=bank;
  if(bank==1 && !(ctl&256))effective=0;
  if(bank==3 && !(ctl&512))effective=2;
  unsigned role=(ctl>>(effective*2))&3;
  for(unsigned kind=0;kind<4;++kind){
   float mask[4];lagi::rbgBankPermissions(ctl,kind,mask);
   for(unsigned edge:{0u,0x1ffffu}){
    auto address=bank*0x20000u+edge;
    assert(lagi::rbgBankReadable(ctl,address,kind)==(role==kind));
    assert(bool(mask[bank])==(role==kind));++checks;
   }
  }
 }
 assert(!lagi::rbgBankReadable(0x13b4,0x80000,2));
 assert(!lagi::rbgBankReadable(0x13b4,0,2));
 assert(lagi::rbgBankReadable(0x13b4,0x60000,2));
 assert(lagi::rbgBankReadable(0x13b4,0x40000,3));
 std::vector<std::uint8_t> raw(0x81000,0xff);
 std::vector<std::uint32_t> pixels(lagi::RbgDecodedPage::Pixels,1);
 lagi::RbgDecodedPage cache;lagi::RbgDecodedPage::Generations gen{};
 lagi::RbgPageKey key{0,0x8008,0,false,true,0x13b4,true};
 auto update=[&]{return cache.update(raw.data(),gen,key,pixels.data(),pixels.size());};
 assert(update()==lagi::RbgPageResult::Rebuilt);
 assert(std::all_of(pixels.begin(),pixels.end(),[](auto p){return p==0;}));
 assert(!cache.dependsOn(0));++gen[0];assert(update()==lagi::RbgPageResult::Hit);
 key.base=0x60000;
 for(unsigned i=0;i<1024;++i){raw[key.base+i*2]=0x50;raw[key.base+i*2+1]=0;}
 assert(update()==lagi::RbgPageResult::Rebuilt);
 assert(pixels[0]==(0xff000000u|0x5ff)); // legal names and character banks.
 assert(cache.dependsOn(0x60000/4096) && cache.dependsOn(0x40000/4096));
 // Register-only role change must invalidate despite unchanged VRAM epochs.
 key.ramctl=0x1384;assert(update()==lagi::RbgPageResult::Rebuilt);
 assert(std::all_of(pixels.begin(),pixels.end(),[](auto p){return p==0;}));
 assert(!cache.dependsOn(0x40000/4096));
 key.ramctl=0x13b4;assert(update()==lagi::RbgPageResult::Rebuilt);assert(pixels[0]);
 printf("PASS %u RAMCTL bank/role/edge checks, raw-uniform permissions, inaccessible name/character pages, cache dependency and register-only invalidation\n",checks);
}

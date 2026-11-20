#include "lagi/native_color_animation.h"
#include <array>
#include <vector>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <string>
#include <functional>
using u8=std::uint8_t;using u32=std::uint32_t;
static FILE* disc;
static std::vector<u8> extent(u32 lba,u32 size){
 std::vector<u8> b(size);
 for(size_t p=0;p<size;p+=2048){auto n=std::min<size_t>(2048,size-p);
  assert(!_fseeki64(disc,(std::uint64_t(lba)+p/2048)*2352+16,SEEK_SET));
  assert(fread(b.data()+p,1,n,disc)==n);
 }return b;
}
static u32 le(const u8* p){return u32(p[0])|u32(p[1])<<8|u32(p[2])<<16|u32(p[3])<<24;}
static std::vector<u8> file(const char* name){
 auto pvd=extent(16,2048);auto root=extent(le(pvd.data()+158),le(pvd.data()+166));
 for(size_t p=0;p<root.size();){unsigned n=root[p];if(!n){p=(p/2048+1)*2048;continue;}
  std::string entry(reinterpret_cast<char*>(root.data()+p+33),root[p+32]);
  if(entry.substr(0,entry.find(';'))==name)return extent(le(root.data()+p+2),le(root.data()+p+10));p+=n;
 }assert(false);return {};
}
struct Light {short normal[3]{};std::uint16_t color[3]{};bool hasColor=false;};
struct Record {unsigned lightingControl=0,lightingCount=0;Light lighting[4];};
int main(int argc,char** argv){
 assert(argc==2);assert(!fopen_s(&disc,argv[1],"rb"));
 const auto bundle=file("FLD_A3_0.MCB");
 auto word=[&](size_t p){assert(p+4<=bundle.size());return lagi::nativeColorWord(bundle.data()+p);};
 auto half=[&](size_t p){assert(p+2<=bundle.size());return unsigned(bundle[p])<<8|bundle[p+1];};
 std::vector<unsigned> polygonCounts;
 std::function<void(u32)> visit=[&](u32 node){while(node){assert(node+12<=bundle.size());
  auto model=word(node);unsigned count=0;
  if(model){size_t p=model+12;while(word(p)||word(p+4)){
   unsigned mode=(half(p+8)>>8)&3; p+=20+(mode==1?8:mode==2?48:mode==3?24:0);++count;
  }}polygonCounts.push_back(count);
  if(auto child=word(node+4))visit(child);node=word(node+8);
 }};visit(word(40));assert(!polygonCounts.empty());
 printf("Disc generator hierarchy: %zu bones\n",polygonCounts.size());
 unsigned frames=0,records=0,nonzero=0;std::vector<int> seenColors;
 for(unsigned slot:{0x300u,0x304u,0x308u}){
  const auto streamOffset=word(slot);assert(streamOffset<bundle.size());
  const auto* stream=bundle.data()+streamOffset;
  const auto count=half(streamOffset);assert(count>0);
  for(unsigned f=0;f<count;++f)for(unsigned bone=0;bone<polygonCounts.size();++bone){
   const auto table=word(streamOffset+4);
   auto boneTable=word(streamOffset+table+f*4);
   auto offset=boneTable?word(streamOffset+boneTable+bone*4):0;
   const auto* data=lagi::nativeColorBone(stream,f,bone);
   assert(data==(offset?stream+offset:nullptr));
   // Prove the actual disc records terminate within the file before invoking
   // the trusted native runtime decoder (which deliberately owns no file IO).
   if(data){size_t p=streamOffset+offset;
    for(unsigned n=0;;++n,p+=16){assert(n<=polygonCounts[bone]);auto index=word(p);
     if(index==0xffffffffu)break;assert(index<polygonCounts[bone] && p+16<=bundle.size());
    }
   }
   assert(lagi::nativeColorRecords(data,polygonCounts[bone],[&](auto c){
    ++records;for(auto& corner:c.rgb)for(auto channel:corner){nonzero+=channel!=0;seenColors.push_back(channel);}
   }));++frames;
  }
  assert(!lagi::nativeColorBone(stream,-1,0));assert(!lagi::nativeColorBone(stream,count,0));
 }
 assert(records>0 && nonzero>0);assert(*std::max_element(seenColors.begin(),seenColors.end())==15);
 // Signed channels, one-normal expansion, replacement of existing material
 // offsets, preservation of normals, and untouched unlit polygons.
 lagi::NativeAnimatedPolygonColor color{};color.rgb[0][0]=-128;color.rgb[3][2]=127;
 Record record{};record.lightingControl=0x105;record.lightingCount=1;record.lighting[0].normal[0]=4096;
 lagi::applyNativeAnimatedColor(record,color);
 assert(record.lightingControl==0x205 && record.lightingCount==4);
 assert(short(record.lighting[0].color[0])==-32768 && record.lighting[3].color[2]==32512);
 for(auto& c:record.lighting)assert(c.normal[0]==4096 && c.hasColor);
 Record unlit{};lagi::applyNativeAnimatedColor(unlit,color);assert(unlit.lightingCount==0);
 std::array<u8,32> asymmetric{};asymmetric[4]=0x80;asymmetric[5]=7;asymmetric[6]=15;
 std::fill(asymmetric.begin()+16,asymmetric.begin()+20,0xff);
 assert(lagi::nativeColorRecords(asymmetric.data(),1,[](auto c){
  assert(c.rgb[0][0]==15 && c.rgb[0][1]==7 && c.rgb[0][2]==-128);
 }));
 std::array<u8,32> bad{};bad[3]=1;bad[19]=1;unsigned emitted=0;
 assert(!lagi::nativeColorRecords(bad.data(),2,[&](auto){++emitted;}));assert(emitted==1);
 bad[3]=2;assert(!lagi::nativeColorRecords(bad.data(),2,[](auto){}));
 fclose(disc);printf("PASS native color animation: %u frame/bone samples, %u sparse records, %u nonzero channels; signed offsets, single-normal expansion, unlit and malformed-index checks\n",frames,records,nonzero);
}

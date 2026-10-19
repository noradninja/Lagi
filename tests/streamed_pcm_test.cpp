#include "lagi/streamed_pcm.h"
#include <cassert>
#include <cstdio>
#include <vector>
#include <string>
#include <thread>
#include <algorithm>
#include <stdexcept>
using u8=std::uint8_t;using u32=std::uint32_t;
static u32 be(const u8* p){return u32(p[0])<<24|u32(p[1])<<16|u32(p[2])<<8|p[3];}
static u32 le(const u8* p){return u32(p[3])<<24|u32(p[2])<<16|u32(p[1])<<8|p[0];}
static std::vector<u8> extent(FILE* file,u32 lba,u32 size){
 std::vector<u8> b(size);for(size_t n=0;n<size;n+=2048){auto z=std::min<size_t>(2048,size-n);
  if(_fseeki64(file,(std::uint64_t(lba)+n/2048)*2352+16,SEEK_SET)||fread(b.data()+n,1,z,file)!=z)throw std::runtime_error("disc read");}return b;
}
static std::vector<u8> program(FILE* f,const char* wanted){
 auto p=extent(f,16,2048);auto root=extent(f,le(p.data()+158),le(p.data()+166));
 for(size_t n=0;n<root.size();){auto z=root[n];if(!z){n=(n/2048+1)*2048;continue;}
  std::string name(reinterpret_cast<const char*>(root.data()+n+33),root[n+32]);name=name.substr(0,name.find(';'));
  if(name==wanted)return extent(f,le(root.data()+n+2),le(root.data()+n+10));n+=z;
 }throw std::runtime_error("missing file");
}
static void units(){
 lagi::StreamedPCM<8> q;using R=lagi::StreamedPCM<8>::Result;
 const u8 pcm[]={0,0,3,232,7,208,11,184};std::int16_t out[32]{};
 q.begin(22050);assert(q.enqueue(pcm,8,1,16)==R::busy);q.mix(out,0,44100);
 assert(q.enqueue(pcm,8,1,16)==R::accepted);std::fill(out,out+16,10);q.mix(out,8,44100);
 const int expected[]={0,500,1000,1500,2000,2500,3000,3000};
 for(int i=0;i<8;++i)assert(out[i*2]==expected[i]+10&&out[i*2+1]==expected[i]+10);
 assert(q.queued()==0);q.begin(44100);q.mix(out,0,44100);
 const u8 stereo[]={0x80,0x7f,1,0xff};assert(q.enqueue(stereo,4,2,8)==R::accepted);
 std::fill(out,out+4,0);q.mix(out,2,44100);assert(out[0]==-32768&&out[1]==256&&out[2]==32512&&out[3]==-256);
 const u8 a[]={0x12},b[]={0x34,0xfe,0xdc};assert(q.enqueue(a,4,2,16,b,1)==R::accepted);
 std::fill(out,out+2,0);q.mix(out,1,44100);assert(out[0]==0x1234&&out[1]==-292);
 const u8 loud[]={0x7f,0xff,0x80,0};q.enqueue(loud,4,1,16);out[0]=out[1]=100;out[2]=out[3]=-100;q.mix(out,2,44100);
 assert(out[0]==32767&&out[2]==-32768);
 assert(q.enqueue(pcm,3,1,16)==R::invalid);assert(q.enqueue(pcm,8,3,16)==R::invalid);
 const u8 full[16]{};assert(q.enqueue(full,16,1,16)==R::accepted);assert(q.enqueue(pcm,2,1,16)==R::busy);
 q.begin(0);q.mix(out,0,44100);assert(q.queued()==0);q.begin(44100);q.mix(out,0,44100);
 assert(q.enqueue(pcm,2,1,16)==R::accepted); // old-generation payload cannot leak
 q.pause(true);out[0]=out[1]=77;q.mix(out,1,44100);assert(out[0]==77&&q.queued()==1);q.pause(false);
 q.begin(44100);assert(q.enqueue(pcm,2,1,16)==R::busy);q.mix(out,0,44100);assert(q.queued()==0);
 std::thread consumer([&]{for(int n=1;n<=10000;){if(!q.queued()){std::this_thread::yield();continue;}
  std::int16_t v[2]{};q.mix(v,1,44100);assert(v[0]==n&&v[1]==n);++n;}});
 for(int n=1;n<=10000;++n){u8 data[]={u8(n>>8),u8(n)};while(q.enqueue(data,2,1,16)!=R::accepted)std::this_thread::yield();}
 consumer.join();assert(q.queued()==0);
 puts("PASS PCM signed 8/BE16 mono/planar stereo, wrapped samples, additive saturation, 22050->44100 interpolation, backpressure, reset/stop and 10000-frame SPSC stress");
}
static void discTest(const char* path){
 FILE* file=nullptr;assert(!fopen_s(&file,path,"rb"));
 for(auto name:{"E006.EPK","E011.EPK","E014.EPK"}){
  auto bytes=program(file,name);const unsigned rate=be(bytes.data()+8)>>16,channels=bytes[4],bits=bytes[5];
  lagi::StreamedPCM<> q;q.begin(rate);q.mix(nullptr,0,rate);
  size_t packets=0;std::uint64_t frames=0;int peak=0;
  for(size_t at=be(bytes.data());at+16<=bytes.size();){auto size=be(bytes.data()+at),opcode=be(bytes.data()+at+4);
   assert(size>=16&&at+size<=bytes.size());
   if(opcode==1){const auto* pcm=bytes.data()+at+16;const size_t payload=size-16, count=payload/(channels*(bits/8));
    assert(q.enqueue(pcm,payload,channels,bits)==lagi::StreamedPCM<>::Result::accepted);
    std::vector<std::int16_t> result(count*2);q.mix(result.data(),count,rate);assert(q.queued()==0);
    for(size_t i=0;i<count;++i)for(unsigned ch=0;ch<2;++ch){auto offset=((channels==2?ch:0)*count+i)*(bits/8);
     const int expected=bits==16?int(std::int16_t(unsigned(pcm[offset])*256+pcm[offset+1])):int(std::int8_t(pcm[offset]))*256;
     assert(result[i*2+ch]==expected);peak=std::max(peak,std::abs(expected));}
    frames+=count;++packets;
   }at+=size;
  }
  printf("PASS original %s rate=%u channels=%u bits=%u packets=%zu PCMframes=%llu seconds=%.3f peak=%d; packet decode only, not listening/sync acceptance\n",name,rate,channels,bits,packets,static_cast<unsigned long long>(frames),double(frames)/rate,peak);
 }fclose(file);
}
int main(int argc,char** argv){units();if(argc>1)discTest(argv[1]);}

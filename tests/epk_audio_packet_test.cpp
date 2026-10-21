#include <cstdint>
#include <cstddef>
#include <algorithm>
#include <cassert>
#include <cstdio>
using u8=std::uint8_t;using u32=std::uint32_t;using s32=int;
static u32 READ_BE_U32(const u8* p){return u32(p[0])<<24|u32(p[1])<<16|u32(p[2])<<8|p[3];}
struct Metadata{u32 m8_sampleRateFP=22050u<<16;unsigned m4_channels=1,m5_bitDepth=16;};
struct sStreamingFile{struct{u8* m34_pBufferRead;u8* m38;u8* m3C;}m28;Metadata* m194;};
namespace lagi::platform::logging{void writef(const char*,...) {}}
int result=0,calls=0,closed=0;const u8* expectedData;const u8* expectedWrap;size_t expectedContiguous;
int lagiSubmitCutscenePCM(const void*,const u8* p,size_t bytes,unsigned rate,unsigned channels,unsigned bits,const u8* wrap,size_t contiguous){
 assert(p==expectedData&&wrap==expectedWrap&&contiguous==expectedContiguous&&bytes==16);
 assert(rate==22050&&channels==1&&bits==16);++calls;return result;
}
void updateStreamingFileReadSub0(sStreamingFile*){++closed;}
#include "packet.inc"
int main(){
 u8 ring[96]{};Metadata meta;sStreamingFile s{{ring+44,ring+64,ring},&meta};
 ring[47]=32;expectedData=ring+60;expectedWrap=ring;expectedContiguous=4;
 assert(cutsceneCommand1(&s)==0&&calls==1);
 result=-1;assert(cutsceneCommand1(&s)==-1&&s.m28.m34_pBufferRead==ring+44&&closed==0);
 // Header itself straddles the ring; parser's header scratch supplies length.
 s.m28.m34_pBufferRead=ring+56;ring[59]=32;expectedData=ring+8;expectedContiguous=16;
 result=0;assert(cutsceneCommand1(&s)==0);
 result=-2;assert(cutsceneCommand1(&s)==-1&&closed==1);
 puts("PASS actual EPK audio packet handler: metadata, payload/header wraps, accepted/busy retry and malformed-format closure; audio sink mocked");
}

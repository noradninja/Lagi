#include "../include/lagi/rbg_decoded_page.h"
#include <cassert>
#include <cstdio>
#include <vector>
#include <cmath>
using std::floor;using std::fmod;
static float frac(float v){return v-std::floor(v);}
#include "../shaders/vdp2_rbg_over_pattern.cginc"
using u8=std::uint8_t;using u16=std::uint16_t;using u32=std::uint32_t;
static std::vector<u8> image(0x81000);
static u8 getVdp2VramU8(u32 a){assert(a<0x80000);return image[a];}
static u16 getVdp2VramU16(u32 a){return (getVdp2VramU8(a)<<8)|getVdp2VramU8(a+1);}
static u16 getVdp2CramU16(u32 a){a&=4095;return (image[0x80000+a]<<8)|image[0x80000+((a+1)&4095)];}
struct s_layerData {u32 mapwh=4,PLSZ=0,planeOffsets[16]{},CHCN=1,CNSM=0,CHSZ=1,SCN=0,CAOS=0;};
struct s_tileDimensions {
    u32 planeDotWidth=512,planeDotHeight=512,pageDotDimension=512;
    u32 characterPatternDotDimension=16,cellDotDimension=8,pageDimension=32;
    u32 pageSize=2048,patternSize=2,cellSizeInByte=64;
};
// Extracted at test time from the installed upstream source, never copied
// into a committed fork. This executes the real Azel address/flip oracle.
#include "native_rbg_reference.inc"
static unsigned quantize(unsigned byte){return (byte*31+127)/255;}
static u32 native555(u32 c){return ((c>>19)&31)|(((c>>11)&31)<<5)|(((c>>3)&31)<<10);}
static u32 cached555(u32 c){return quantize(c&255)|(quantize((c>>8)&255)<<5)|(quantize((c>>16)&255)<<10);}
int main() {
    for(unsigned i=0;i<image.size();++i)image[i]=u8((i*37)^(i>>5));
    constexpr unsigned base=0x70000;
    for(unsigned i=0;i<1024;++i) {
        unsigned name=(i&1023)|(((i/7)&3)<<10)|(((i/11)&7)<<12);
        image[base+i*2]=u8(name>>8);image[base+i*2+1]=u8(name);
    }
    std::vector<u32> pixels(lagi::RbgDecodedPage::Pixels);
    lagi::RbgDecodedPage cache;lagi::RbgDecodedPage::Generations generations{};generations.fill(1);
    s_layerData layer;for(auto& offset:layer.planeOffsets)offset=base;
    s_tileDimensions dimensions;
    for(unsigned scn: {0u,1u,2u,3u,4u,7u,8u,11u})for(unsigned caos=0;caos<8;++caos) {
        lagi::RbgPageKey key{base,u16(0x8000|scn),u16(caos),false};
        assert(cache.update(image.data(),generations,key,pixels.data(),pixels.size())==lagi::RbgPageResult::Rebuilt);
        layer.SCN=scn;layer.CAOS=caos;
        for(unsigned y=0;y<512;++y)for(unsigned x=0;x<512;++x) {
            auto native=sampleTileAtCoordinate(x,y,layer,dimensions),decoded=pixels[y*512+x];
            assert(bool(native>>24)==bool(decoded>>24));
            if(native)assert(native555(native)==cached555(decoded));
        }
        assert(cache.update(image.data(),generations,key,pixels.data(),pixels.size())==lagi::RbgPageResult::Hit);
    }
    auto key=cache.key();
    assert(!cache.dependsOn(126)); // Stand-in for changing, unrelated coefficients.
    ++generations[126];
    assert(cache.update(image.data(),generations,key,pixels.data(),pixels.size())==lagi::RbgPageResult::Hit);
    const unsigned firstCharacterBlock=((key.pncr&3)|((key.pncr&0x1c)<<10))*32/4096;
    for(unsigned block: {base/4096,firstCharacterBlock,128u}) {
        assert(cache.dependsOn(block));++generations[block];
        assert(cache.update(image.data(),generations,key,pixels.data(),pixels.size())==lagi::RbgPageResult::Rebuilt);
    }
    // Native R0TPON override: opaque dot zero still uses palette entry zero.
    image[0]=0;image[base]=image[base+1]=0;++generations[base/4096];++generations[0];
    key.pncr=0x8000;key.opaqueZero=true;
    assert(cache.update(image.data(),generations,key,pixels.data(),pixels.size())==lagi::RbgPageResult::Rebuilt);
    assert(pixels[0]>>24);
    key.pncr=0x801f;
    assert(cache.update(image.data(),generations,key,pixels.data(),pixels.size())==lagi::RbgPageResult::Unsupported);
    assert(!cache.ready());
    assert(cache.update(image.data(),generations,key,pixels.data(),pixels.size())==lagi::RbgPageResult::Unsupported);
    key.base=0x70001;
    assert(cache.update(image.data(),generations,key,pixels.data(),pixels.size())==lagi::RbgPageResult::Unsupported);
    key={base,0x8000,7,false,true};layer.SCN=0;layer.CAOS=7;
    assert(cache.update(image.data(),generations,key,pixels.data(),pixels.size())==lagi::RbgPageResult::Rebuilt);
    for(unsigned y=0;y<512;++y)for(unsigned x=0;x<512;++x) {
        auto native=sampleTileAtCoordinate(x,y,layer,dimensions),entry=pixels[y*512+x];
        assert(bool(native>>24)==bool(entry>>24));
        if(native)assert(native555(native)==(getVdp2CramU16(((entry&2047)+layer.CAOS*256)*2)&0x7fff));
    }
    assert(!cache.dependsOn(128));++generations[128];
    assert(cache.update(image.data(),generations,key,pixels.data(),pixels.size())==lagi::RbgPageResult::Hit);
    // Repeated names exercise page-local reuse, including flip/palette bits.
    // Alternating many names also forces direct-map collisions: equality must
    // use the full pattern name, never just the bucket.
    for(unsigned tile=0;tile<1024;++tile) {
        const unsigned n=(tile%73)|(((tile/73)&3)<<10)|(((tile/29)&7)<<12);
        image[base+tile*2]=u8(n>>8);image[base+tile*2+1]=u8(n);
    }
    ++generations[base/4096];
    for(bool indexed:{false,true})for(bool opaque:{false,true}) {
        key.indexed=indexed;key.opaqueZero=opaque;
        assert(cache.update(image.data(),generations,key,pixels.data(),pixels.size())==lagi::RbgPageResult::Rebuilt);
        for(unsigned y=0;y<512;++y)for(unsigned x=0;x<512;++x) {
            const auto native=sampleTileAtCoordinate(x,y,layer,dimensions),entry=pixels[y*512+x];
            if(native)assert(native555(native)==(indexed?
                (getVdp2CramU16(((entry&2047)+layer.CAOS*256)*2)&0x7fff):cached555(entry)));
            if(!opaque)assert(bool(native>>24)==bool(entry>>24));
        }
    }
    // Run the production shader's scalar OVPNR address routine against the
    // original tile decoder. A register-selected name bypasses name VRAM;
    // place that name at the oracle's first tile to compare character pixels.
    for(unsigned scn:{0u,1u,2u,3u,4u,7u,8u,11u}) {
        layer.SCN=scn;
        for(unsigned name=0;name<65536;++name) {
            const unsigned character=((name&1023)*4)|(scn&3)|((scn&28)<<10);
            if(character*32>0x80000-256)continue;
            image[base]=u8(name>>8);image[base+1]=u8(name);
            for(unsigned y=0;y<16;++y)for(unsigned x=0;x<16;++x) {
                const unsigned address=unsigned(rbgOverPatternDotAddress(float(name),float(scn),float(x),float(y)));
                assert(address>=character*32 && address<character*32+256);
                const unsigned dot=image[address];
                const auto native=sampleTileAtCoordinate(x,y,layer,dimensions);
                assert(bool(native>>24)==bool(dot));
                if(dot)assert(native555(native)==(getVdp2CramU16(
                    (layer.CAOS*256+((name&0x7000)>>4)+dot)*2)&0x7fff));
                // Screen-over uses positive modulo for negative coordinates.
                assert(address==unsigned(rbgOverPatternDotAddress(float(name),float(scn),float(x)-32,float(y)-32)));
            }
        }
    }
    std::puts("PASS native decoded page oracle and production OVPNR addresses: all pattern words, flips, palette wrap, signed repeat, invalidation and fallback");
}

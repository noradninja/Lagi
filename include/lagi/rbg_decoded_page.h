#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace lagi {
// Resource decoding only. Rotation, windows, priorities and composition do
// not belong to this cache. One native 16x16/8-bit/one-word page is 512x512.
struct RbgPageKey {
    std::uint32_t base=0;
    std::uint16_t pncr=0,craofb=0;
    bool opaqueZero=false;
    bool indexed=false; // Retain palette index; animated CRAM stays live on GPU.
    bool operator==(const RbgPageKey&) const = default;
};
enum class RbgPageResult { Hit, Rebuilt, Unsupported };
class RbgDecodedPage {
public:
    static constexpr unsigned Width=512,Pixels=Width*Width,Blocks=129;
    using Generations=std::array<std::uint32_t,Blocks>;
    void reset(){known_=false;ready_=false;}
    bool dependsOn(unsigned block) const {return block<Blocks && dependencies_[block];}
    const RbgPageKey& key() const {return key_;}
    bool ready() const {return ready_;}
    RbgPageResult update(const std::uint8_t* raw,const Generations& generations,
        const RbgPageKey& key,std::uint32_t* pixels,std::size_t capacity) {
        if(!raw || !pixels || capacity<Pixels)return RbgPageResult::Unsupported;
        bool unchanged=known_ && key==key_;
        if(unchanged)for(unsigned i=0;i<Blocks;++i)
            if(dependencies_[i] && generations[i]!=generations_[i]){unchanged=false;break;}
        if(unchanged)return ready_?RbgPageResult::Hit:RbgPageResult::Unsupported;
        known_=true;ready_=false;key_=key;
        dependencies_.fill(false);generations_=generations;
        if((key.base&2047u) || key.base>0x80000u-2048u || (key.pncr&0xC000u)!=0x8000u)
            return RbgPageResult::Unsupported;
        depend(key.base,2048); // Pattern names: changing them can change dependencies.
        if(!key.indexed)depend(0x80000,4096);
        std::array<std::uint32_t,2048> colors{};
        const unsigned scn=key.pncr&0x3ff,caos=(key.craofb&7)*256;
        // Resolve format, palette wrap and transparent dot zero once per
        // palette entry, not once per output pixel. Indexed pages still keep
        // CAOS/CRAM live on the GPU and do not depend on CPU palette data.
        for(unsigned i=0;i<colors.size();++i)
            colors[i]=(!(i&255) && !key.opaqueZero)?0:
                (key.indexed?(0xff000000u|i):rgba(word(raw+0x80000+((i+caos)&2047)*2)));
        // Memoize exact pattern names within this rebuild. The destination
        // page itself owns the decoded tiles: no persistent pixel cache or
        // hash-only equality, and all dependencies are still collected below.
        std::array<unsigned,256> decodedTiles{};
        for(unsigned tile=0;tile<1024;++tile) {
            const unsigned name=word(raw+key.base+tile*2);
            // renderer_vdp2.cpp::sampleTileAtCoordinate: CHSZ=1, CHCN=1,
            // PNB=1, CNSM=0. Character addresses are in 32-byte units.
            const unsigned character=((name&0x3ff)<<2)|(scn&3)|((scn&0x1c)<<10);
            const unsigned characterBase=character*32;
            if(characterBase>0x80000u-256u)return RbgPageResult::Unsupported;
            depend(characterBase,256);
            const unsigned palette=(name&0x7000)>>4,flip=(name>>10)&3;
            const unsigned tileX=(tile%32)*16,tileY=(tile/32)*16;
            const unsigned bucket=((name*2654435761u)>>24);
            const unsigned prior=decodedTiles[bucket];
            if(prior && word(raw+key.base+(prior-1)*2)==name) {
                const unsigned source=((prior-1)/32)*16*Width+((prior-1)%32)*16;
                for(unsigned y=0;y<16;++y)
                    std::memcpy(pixels+(tileY+y)*Width+tileX,
                        pixels+source+y*Width,16*sizeof(*pixels));
                continue;
            }
            decodedTiles[bucket]=tile+1;
            const auto* offsets=characterOffsets[flip].data();
            for(unsigned y=0;y<16;++y)for(unsigned x=0;x<16;++x) {
                const unsigned dot=raw[characterBase+offsets[y*16+x]];
                pixels[(tileY+y)*Width+tileX+x]=colors[palette|dot];
            }
        }
        ready_=true;
        return RbgPageResult::Rebuilt;
    }
private:
    // Exact native four-cell character layout for all flip combinations.
    // Immutable 1 KiB table, computed at compile time; no per-tile allocation.
    inline static constexpr auto characterOffsets=[] {
        std::array<std::array<std::uint8_t,256>,4> result{};
        for(unsigned flip=0;flip<4;++flip)for(unsigned y=0;y<16;++y)for(unsigned x=0;x<16;++x) {
            unsigned px=x,py=y;
            if(flip&2)py=(py&8)?15-py:23-py;
            else if(py&8)py+=8;
            if(flip&1){if(!(px&8))py+=8;px=7-(px&7);}
            else {if(px&8)py+=8;px&=7;}
            result[flip][y*16+x]=static_cast<std::uint8_t>(py*8+px);
        }
        return result;
    }();
    RbgPageKey key_{};
    Generations generations_{};
    std::array<bool,Blocks> dependencies_{};
    bool known_=false,ready_=false;
    void depend(unsigned start,unsigned bytes) {
        for(unsigned b=start/4096;b<=(start+bytes-1)/4096;++b)dependencies_[b]=true;
    }
    static unsigned word(const std::uint8_t* p){return (unsigned(p[0])<<8)|p[1];}
    static std::uint32_t rgba(unsigned c) {
        // Match RGB555 shader output stored in an RGBA8 render surface.
        auto channel=[](unsigned v){return (v*255+15)/31;};
        return 0xff000000u|channel(c&31)|(channel((c>>5)&31)<<8)|(channel((c>>10)&31)<<16);
    }
};
} // namespace lagi

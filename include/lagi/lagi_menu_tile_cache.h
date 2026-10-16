#pragma once
#include <array>
#include <cstdint>
#include <cstring>
#include <cstddef>

namespace lagi {
struct MenuTileSources {
    const std::uint8_t* characters=nullptr;
    std::size_t characterBytes=0;
    const std::uint8_t* palette=nullptr;
    std::size_t paletteBytes=0;
};
// Render-owned cache. Input VRAM/CRAM must remain immutable between beginFrame
// calls, as in Neptune's published snapshot. Validate exact source bytes once
// per touched tile per frame; scroll/map edits only change the selected key.
template<std::size_t Slots=1024> class MenuTileCache {
    static_assert(Slots && !(Slots&(Slots-1)));
    struct Entry {
        std::uint32_t key=0;
        std::uint64_t frame=0;
        bool valid=false;
        std::size_t characterBytes=0,paletteBytes=0;
        std::array<std::uint8_t,256> characters{};
        std::array<std::uint8_t,512> palette{};
        std::array<std::uint32_t,256> pixels{};
    };
    std::array<Entry,Slots> entries{};
    std::uint64_t frame=0;
    unsigned decoded=0;
public:
    void beginFrame() {
        decoded=0;
        if(++frame==0) {for(auto& e:entries)e.valid=false;frame=1;}
    }
    unsigned decodedTiles()const{return decoded;}
    template<class Sources,class Decode>
    const std::uint32_t* tile(std::uint32_t key,Sources sources,Decode decode) {
        auto mixed=key;
        mixed^=mixed>>16;mixed*=0x7FEB352Du;mixed^=mixed>>15;
        auto& e=entries[mixed&(Slots-1)];
        const bool same=e.valid && e.key==key;
        if(!same || e.frame!=frame) {
            const auto s=sources();
            // Caller supplies bounded spans; never accept a truncated cache
            // identity that would hide relevant source changes.
            if(s.characterBytes>e.characters.size() || s.paletteBytes>e.palette.size())
                return nullptr;
            const bool dirty=!same || e.characterBytes!=s.characterBytes || e.paletteBytes!=s.paletteBytes ||
                (s.characterBytes && std::memcmp(e.characters.data(),s.characters,s.characterBytes)) ||
                (s.paletteBytes && std::memcmp(e.palette.data(),s.palette,s.paletteBytes));
            if(dirty) {
                if(s.characterBytes)std::memcpy(e.characters.data(),s.characters,s.characterBytes);
                if(s.paletteBytes)std::memcpy(e.palette.data(),s.palette,s.paletteBytes);
                e.characterBytes=s.characterBytes;e.paletteBytes=s.paletteBytes;
                for(unsigned y=0;y<16;++y)for(unsigned x=0;x<16;++x)e.pixels[y*16+x]=decode(x,y);
                ++decoded;
            }
            e.key=key;e.valid=true;e.frame=frame;
        }
        return e.pixels.data();
    }
};
}

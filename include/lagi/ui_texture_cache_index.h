#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <unordered_map>
namespace lagi {
inline std::uint64_t ui_texture_key(std::uint16_t pmod,std::uint16_t colr,
    std::uint16_t srca,std::uint16_t size)
{
    return std::uint64_t(pmod) | (std::uint64_t(colr)<<16) |
        (std::uint64_t(srca)<<32) | (std::uint64_t(size)<<48);
}
class UiTextureCacheIndex {
    std::unordered_map<std::uint64_t,std::size_t> entries;
public:
    bool find(std::uint64_t key,std::size_t& index) const {
        const auto it=entries.find(key);
        if(it==entries.end())return false;
        index=it->second;return true;
    }
    void insert(std::uint64_t key,std::size_t index){entries[key]=index;}
};
struct UiSpriteProfile {
    unsigned draws=0,worldTagged=0,shaded=0,cacheHits=0,cacheMisses=0,paletteRefreshes=0,slotSkips=0;
    std::uint64_t bboxPixels=0;
};
// Conservative clipped AABB, not true covered fragments or GPU time. Includes
// transparent holes and overlap; summed coverage may exceed one screen.
inline std::uint64_t sprite_bbox_pixels(const float (&p)[4][2],unsigned width,unsigned height)
{
    float left=p[0][0],right=left,bottom=p[0][1],top=bottom;
    for(unsigned i=1;i<4;++i){
        left=std::min(left,p[i][0]);right=std::max(right,p[i][0]);
        bottom=std::min(bottom,p[i][1]);top=std::max(top,p[i][1]);
    }
    left=std::clamp(left,-1.0f,1.0f);right=std::clamp(right,-1.0f,1.0f);
    bottom=std::clamp(bottom,-1.0f,1.0f);top=std::clamp(top,-1.0f,1.0f);
    return static_cast<std::uint64_t>(std::max(0.0f,right-left)*
        std::max(0.0f,top-bottom)*0.25f*width*height);
}
} // namespace lagi

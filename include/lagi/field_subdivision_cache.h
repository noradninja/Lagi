#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace lagi {
// A slot describes the actual XYZ payload in one mapped subdivision quad.
// Compare exact corner bits, not a hash, model pointer or approximate float.
struct FieldSubdivPositionSlot {
    std::array<float,12> corners{};
    bool valid=false;
    bool matches(const std::array<float,12>& value) const {
        return valid && std::memcmp(corners.data(),value.data(),
            sizeof(float)*value.size())==0;
    }
    void remember(const std::array<float,12>& value) {
        std::memcpy(corners.data(),value.data(),sizeof(float)*value.size());
        valid=true;
    }
    void invalidate(){valid=false;}
};

struct FieldSubdivUvGrid {
    float u[9]{},v[9]{};
};
// A fixed-size direct-mapped cache of exact dimension/flip templates. A
// collision is a miss; no texture identity, texels or shade are cached.
class FieldSubdivUvCache {
    struct Entry {
        unsigned width=0,height=0,flip=0;
        FieldSubdivUvGrid grid{};
    };
    std::array<Entry,64> entries{};
public:
    template<class Fill>
    const FieldSubdivUvGrid& get(unsigned width,unsigned height,unsigned flip,
                                 Fill fill,bool& hit) {
        flip&=3u;
        auto& e=entries[(width*73856093u ^ height*19349663u ^ flip*83492791u)&63u];
        hit=e.width==width && e.height==height && e.flip==flip;
        if(!hit) {
            const float u0=0.5f/static_cast<float>(width);
            const float v0=0.5f/static_cast<float>(height);
            const float u1=(static_cast<float>(width)-0.5f)/static_cast<float>(width);
            const float v1=(static_cast<float>(height)-0.5f)/static_cast<float>(height);
            const float uv[4][2]={{u0,v0},{u1,v0},{u1,v1},{u0,v1}};
            static constexpr unsigned order[4][4]={{0,1,2,3},{1,0,3,2},{3,2,1,0},{2,3,0,1}};
            const auto& o=order[flip];
            fill(uv[o[0]][0],uv[o[1]][0],uv[o[2]][0],uv[o[3]][0],e.grid.u);
            fill(uv[o[0]][1],uv[o[1]][1],uv[o[2]][1],uv[o[3]][1],e.grid.v);
            e.width=width;e.height=height;e.flip=flip;
        }
        return e.grid;
    }
};
}

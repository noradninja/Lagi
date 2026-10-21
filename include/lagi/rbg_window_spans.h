#pragma once
#include <algorithm>
#include <array>
#include <cstdint>

namespace lagi {
constexpr unsigned RbgWindowMaxSpans=224u*3u;
// Partition each scanline at the two windows' boundaries. The native WCTLD
// predicate is constant between boundaries, so this replaces a 352-pixel
// CPU scan without moving rotation or coefficient decisions off the GPU.
// A and B are mutually exclusive parameters, not transparent layers.
template<class Emit> bool emitRbgWindowSpans(const float* ctrl,
    const unsigned char* raw,Emit emit) {
    const unsigned control=static_cast<unsigned>(ctrl[4]);
    const unsigned lineMask=static_cast<unsigned>(ctrl[7]);
    struct Window {int first,last;bool enabled,insideArea;};
    for(int y=0;y<224;++y) {
        Window windows[2]{};
        std::array<int,6> cuts{0,352,352,352,352,352};
        unsigned cutCount=2;
        for(unsigned w=0;w<2;++w) {
            auto& window=windows[w];
            window.enabled=(control&(w?8u:2u))!=0;
            window.insideArea=(control&(w?4u:1u))!=0;
            window.first=1;window.last=0;
            if(!window.enabled)continue;
            const unsigned base=8+w*4;
            if(y<static_cast<int>(ctrl[base+1]) || y>static_cast<int>(ctrl[base+3]))continue;
            int first=static_cast<int>(ctrl[base]),last=static_cast<int>(ctrl[base+2]);
            if(lineMask&(1u<<w)) {
                if(!raw)return false;
                const unsigned addr=(static_cast<unsigned>(ctrl[5+w])+unsigned(y)*4)&0x7ffff;
                auto word=[&](unsigned offset){
                    return unsigned(raw[(addr+offset)&0x7ffff]) |
                        (unsigned(raw[(addr+offset+1)&0x7ffff])<<8);
                };
                const unsigned start=word(0),end=word(2);
                if(end==0xffff)continue; // Native disabled-line sentinel.
                first=int((start>>1)&511);last=int((end>>1)&511);
            }
            if(last<first)continue;
            window.first=first;window.last=last;
            cuts[cutCount++]=std::clamp(first,0,352);
            cuts[cutCount++]=std::clamp(last+1,0,352);
        }
        std::sort(cuts.begin(),cuts.begin()+cutCount);
        const auto end=std::unique(cuts.begin(),cuts.begin()+cutCount);
        cutCount=static_cast<unsigned>(end-cuts.begin());
        auto accepted=[&](unsigned w,int x){
            const auto& window=windows[w];
            if(!window.enabled)return 3;
            const bool inside=x>=window.first && x<=window.last;
            return inside==window.insideArea?1:0;
        };
        unsigned parameter=2;
        int spanStart=0,spanEnd=0;
        for(unsigned c=0;c+1<cutCount;++c) {
            const int first=cuts[c],last=cuts[c+1];
            const int a=accepted(0,first),b=accepted(1,first);
            const bool useA=(a&2)?(b&1):(b&2)?(a&1):
                (control&0x80)?(a || b):(a && b);
            const unsigned selected=useA?0u:1u;
            if(selected!=parameter) {
                if(parameter<2)emit(parameter,y,spanStart,spanEnd-1);
                parameter=selected;spanStart=first;
            }
            spanEnd=last;
        }
        if(parameter<2)emit(parameter,y,spanStart,spanEnd-1);
    }
    return true;
}
} // namespace lagi

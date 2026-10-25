#pragma once
#include <array>
#include <cstdint>
namespace lagi {
using NativeRayQuad=std::array<std::array<std::int32_t,3>,4>;
// MAIN 0602CA08..2C: DIV0S, eight DIV1, ROTCL, EXTU.B, SHLL8.
// Operands have already undergone the native SHLL8 (including wrap).
constexpr std::uint32_t nativeClipFraction(std::uint32_t numerator,std::uint32_t denominator) {
    std::uint32_t n=numerator<<8,d=denominator<<8;
    bool q=(n>>31)!=0,m=(d>>31)!=0,t=q!=m;
    for(unsigned step=0;step<8;++step) {
        const bool oldQ=q;q=(n>>31)!=0;n=(n<<1)|unsigned(t);
        const auto before=n;
        if(!oldQ) {
            if(!m){n-=d;const bool borrow=n>before;q=q?!borrow:borrow;}
            else {n+=d;const bool carry=n<before;q=q?carry:!carry;}
        } else {
            if(!m){n+=d;const bool carry=n<before;q=q?!carry:carry;}
            else {n-=d;const bool borrow=n>before;q=q?borrow:!borrow;}
        }
        t=q==m;
    }
    return (((n<<1)|unsigned(t))&255u)<<8;
}
// MAIN 0602C7E6..0602CA98. Return 0 unchanged, 1 clipped, 2 rejected.
// This is the native fixed four-vertex action table, not polygon tessellation.
inline unsigned nativeClipQuadPlane(NativeRayQuad& quad,unsigned axis,
    std::int32_t bound,bool lower)
{
    unsigned mask=0;
    for(unsigned i=0;i<4;++i)
        if(lower?quad[i][axis]<bound:quad[i][axis]>bound)mask|=1u<<i;
    if(!mask)return 0;
    if(mask==5 || mask==10 || mask==15)return 2;
    const auto sub=[](std::int32_t a,std::int32_t b){return std::uint32_t(a)-std::uint32_t(b);};
    const auto edge=[&](unsigned outside,unsigned inside) {
        const auto fraction=nativeClipFraction(sub(bound,quad[outside][axis]),
            sub(quad[inside][axis],quad[outside][axis]));
        const unsigned other=axis^1u;
        const auto delta=static_cast<std::int32_t>(sub(quad[inside][other],quad[outside][other]));
        const auto product=std::int64_t(delta)*fraction;
        quad[outside][other]=static_cast<std::int32_t>(std::uint32_t(quad[outside][other])+
            std::uint32_t(product>>16));
        quad[outside][axis]=bound;
    };
    const auto single=[&](unsigned out,unsigned a,unsigned b) {
        const bool selectB=lower?quad[a][axis]<quad[b][axis]:quad[a][axis]>quad[b][axis];
        edge(out,selectB?b:a);
    };
    switch(mask) {
    case 1:single(0,1,3);break;
    case 2:single(1,0,2);break;
    case 3:edge(0,3);edge(1,2);break;
    case 4:single(2,3,1);break;
    case 6:edge(1,0);edge(2,3);break;
    case 7:edge(1,3);edge(2,3);edge(0,3);break;
    case 8:single(3,2,0);break;
    case 9:edge(0,1);edge(3,2);break;
    case 11:edge(0,2);edge(3,2);edge(1,2);break;
    case 12:edge(3,0);edge(2,1);break;
    case 13:edge(0,1);edge(3,1);edge(2,1);break;
    case 14:edge(1,0);edge(2,0);edge(3,0);break;
    }
    return 1;
}
// MAIN 0602D946: right, left, top, bottom, stopping at rejection.
inline bool nativeClipRayQuad(NativeRayQuad& quad,std::int16_t left,
    std::int16_t right,std::int16_t top,std::int16_t bottom) {
    return nativeClipQuadPlane(quad,0,right,false)!=2 &&
        nativeClipQuadPlane(quad,0,left,true)!=2 &&
        nativeClipQuadPlane(quad,1,top,false)!=2 &&
        nativeClipQuadPlane(quad,1,bottom,true)!=2;
}
}

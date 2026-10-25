#pragma once
#include <array>
#include <cstdint>
namespace lagi {
using NativeRayQuad=std::array<std::array<std::int32_t,3>,4>;
// MAIN 0602CA08..2C: DIV0S, eight DIV1, ROTCL, EXTU.B, SHLL8.
// Operands undergo the native SHLL8 here (including wrap). Near-depth
// clipping selects DIV0U initialization instead of DIV0S.
constexpr std::uint32_t nativeClipFraction(std::uint32_t numerator,std::uint32_t denominator,bool signedDivision=true) {
    std::uint32_t n=numerator<<8,d=denominator<<8;
    bool q=signedDivision && (n>>31)!=0,m=signedDivision && (d>>31)!=0,t=q!=m;
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
// MAIN 0602C7E6..0602CAF2. Return 0 unchanged, 1 clipped, 2 rejected.
// This is the native fixed four-vertex action table, not polygon tessellation.
// Axes 0/1 are signed screen clipping; axis 2 is unsigned near-depth clipping.
inline unsigned nativeClipQuadPlaneBounds(NativeRayQuad& quad,unsigned axis,
    std::int32_t visibleBound,std::int32_t guardBound,bool lower,std::array<float,4>* inverseDepth=nullptr)
{
    unsigned mask=0;
    for(unsigned i=0;i<4;++i)
        if(lower?quad[i][axis]<visibleBound:quad[i][axis]>visibleBound)mask|=1u<<i;
    if(!mask)return 0;
    if(mask==5 || mask==10 || mask==15)return 2;
    const auto sub=[](std::int32_t a,std::int32_t b){return std::uint32_t(a)-std::uint32_t(b);};
    const auto edge=[&](unsigned outside,unsigned inside) {
        // Native screen callbacks C9F0/C9FE/CA44/CA52 leave a classified
        // outside vertex unchanged while it still lies within the guard band.
        if(lower?quad[outside][axis]>=guardBound:quad[outside][axis]<=guardBound)return;
        const auto fraction=nativeClipFraction(sub(guardBound,quad[outside][axis]),
            sub(quad[inside][axis],quad[outside][axis]),axis!=2);
        const auto interpolate=[&](unsigned other) {
            const auto delta=static_cast<std::int32_t>(sub(quad[inside][other],quad[outside][other]));
            const auto product=std::int64_t(delta)*fraction;
            quad[outside][other]=static_cast<std::int32_t>(std::uint32_t(quad[outside][other])+
                std::uint32_t(product>>16));
        };
        // Renderer-only depth follows the same quantized screen edge. It
        // never participates in native XY decisions or packet sorting.
        if(inverseDepth)(*inverseDepth)[outside]+=((*inverseDepth)[inside]-(*inverseDepth)[outside])*
            (float(fraction)/65536.0f);
        if(axis==2){interpolate(0);interpolate(1);}
        else interpolate(axis^1u);
        quad[outside][axis]=guardBound;
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
inline unsigned nativeClipQuadPlane(NativeRayQuad& quad,unsigned axis,
    std::int32_t bound,bool lower,std::array<float,4>* inverseDepth=nullptr) {
    return nativeClipQuadPlaneBounds(quad,axis,bound,bound,lower,inverseDepth);
}
// MAIN 0602C848 and 0602C7C6: near clips the native four-corner shape;
// far rejects the whole primitive if any resulting vertex reaches the limit.
inline bool nativeClipPolygonDepth(NativeRayQuad& quad,std::int32_t near,std::int32_t far) {
    if(nativeClipQuadPlane(quad,2,near,true)==2)return false;
    for(const auto& p:quad)if(p[2]>=far)return false;
    return true;
}
struct NativeScreenClipBounds {std::int16_t left,right,top,bottom;};
// MAIN 0602D946 and A7 060699FC: classify against visible C/E/8/A,
// intersect against guard 4/6/0/2. Keep native right/left/top/bottom order.
inline bool nativeClipSceneQuad(NativeRayQuad& quad,const NativeScreenClipBounds& visible,
    const NativeScreenClipBounds& guard,std::array<float,4>* inverseDepth=nullptr) {
    return nativeClipQuadPlaneBounds(quad,0,visible.right,guard.right,false,inverseDepth)!=2 &&
        nativeClipQuadPlaneBounds(quad,0,visible.left,guard.left,true,inverseDepth)!=2 &&
        nativeClipQuadPlaneBounds(quad,1,visible.top,guard.top,false,inverseDepth)!=2 &&
        nativeClipQuadPlaneBounds(quad,1,visible.bottom,guard.bottom,true,inverseDepth)!=2;
}
// Single-bound form for callers whose classification/intersection bounds agree.
inline bool nativeClipRayQuad(NativeRayQuad& quad,std::int16_t left,
    std::int16_t right,std::int16_t top,std::int16_t bottom,std::array<float,4>* inverseDepth=nullptr) {
    const NativeScreenClipBounds bounds{left,right,top,bottom};
    return nativeClipSceneQuad(quad,bounds,bounds,inverseDepth);
}
// MAIN 0602D912: near clip mutates the two endpoints, then far rejects
// if either endpoint is at or beyond the limit. Near intersections use
// DIV0U (0602CA9C), unlike the signed XY screen intersections above.
inline bool nativeClipRayDepth(std::array<std::array<std::int32_t,3>,2>& line,
    std::int32_t near,std::int32_t far) {
    const bool out0=line[0][2]<near,out1=line[1][2]<near;
    if(out0 && out1)return false;
    if(out0 || out1) {
        auto& out=line[out0?0:1];const auto& in=line[out0?1:0];
        const auto fraction=nativeClipFraction(std::uint32_t(near)-std::uint32_t(out[2]),
            std::uint32_t(in[2])-std::uint32_t(out[2]),false);
        for(unsigned axis=0;axis<2;++axis) {
            const auto delta=static_cast<std::int32_t>(std::uint32_t(in[axis])-std::uint32_t(out[axis]));
            out[axis]=static_cast<std::int32_t>(std::uint32_t(out[axis])+
                std::uint32_t((std::int64_t(delta)*fraction)>>16));
        }
        out[2]=near;
    }
    return line[0][2]<far && line[1][2]<far;
}
}

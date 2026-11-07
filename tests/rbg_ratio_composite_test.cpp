#include "lagi/vdp2_sprite_color.h"
#include <cassert>
#include <cmath>
#include <algorithm>
#include <cstdio>
// Minimal host scalar/vector shim. The PS1 strips only Cg semantics and
// includes the actual production shader; it does not rewrite its decisions.
struct float2 {float x,y;float2(float a,float b):x(a),y(b){}};
struct float3 {float x,y,z;float3(float a):x(a),y(a),z(a){}
    float3(float a,float b,float c):x(a),y(b),z(c){}};
float3 operator*(float3 a,float b){return {a.x*b,a.y*b,a.z*b};}
float3 operator/(float3 a,float b){return a*(1/b);}
float3 operator+(float3 a,float b){return {a.x+b,a.y+b,a.z+b};}
float3 operator+(float3 a,float3 b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
float3 floor(float3 a){return {std::floor(a.x),std::floor(a.y),std::floor(a.z)};}
struct float4 {float x,y,z,w,a,g,b;float2 xy;float3 rgb;
    float4(float q,float r,float s,float t):x(q),y(r),z(s),w(t),a(t),g(r),b(s),xy(q,r),rgb(q,r,s){}
    float4(float3 c,float t):float4(c.x,c.y,c.z,t){}
};
float4 operator*(float4 a,float b){return {a.x*b,a.y*b,a.z*b,a.w*b};}
float4 operator+(float4 a,float b){return {a.x+b,a.y+b,a.z+b,a.w+b};}
float4 floor(float4 a){return {std::floor(a.x),std::floor(a.y),std::floor(a.z),std::floor(a.w)};}
float3 min(float3 a,float b){return {std::min(a.x,b),std::min(a.y,b),std::min(a.z,b)};}
float2 operator*(float2 a,float2 b){return {a.x*b.x,a.y*b.y};}
using sampler2D=unsigned;
static float4 samples[4]={{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}};
float4 tex2D(sampler2D id,float2){return samples[id];}
using std::max;
#define uniform
#define main shade
#include "rbg_ratio_composite_host.inc"
#undef main
static float3 color(unsigned w){return {float(w&31)/31,float((w>>5)&31)/31,float((w>>10)&31)/31};}
static unsigned word(float3 c){return unsigned(std::round(c.x*31))|
    (unsigned(std::round(c.y*31))<<5)|(unsigned(std::round(c.z*31))<<10);}
int main(){
    lagi::Vdp2SpriteColorState s;
    s.spctl=0x20;s.ccctl=0x10;s.bgon=0x19;s.prir=5;s.prina=2;s.prinb=0x700;
    s.priority[0]=0x404;s.priority[1]=0x407;
    assert(lagi::rbgLayerCompositeSupported(s));
    assert(lagi::spriteUiPrioritySupported(s));
    assert(lagi::spritePixelPriority(s,145)==7); // bank2 wins NBG3's priority7 tie
    assert((lagi::spriteCompositeLookup(s,145)>>24)==7);
    const auto base=s;
    for(unsigned bit:{2u,4u,0x20u}){s=base;s.bgon|=bit;assert(!lagi::rbgLayerCompositeSupported(s));}
    for(unsigned bit:{1u,2u,0x100u,0x200u}){
        s=base;s.specialPriority=bit;assert(!lagi::rbgLayerCompositeSupported(s));
        s=base;s.specialColor=bit;assert(!lagi::rbgLayerCompositeSupported(s));
    }
    s=base;s.ccctl|=1;assert(!lagi::rbgLayerCompositeSupported(s));
    // Actual Camp night registers: RBG0 priority3, NBG1 priority6,
    // NBG3 priority7; CCCTL enables NBG1+RBG0+sprite and additive mode.
    s={};s.ccctl=0x152;s.spctl=0x1223;s.bgon=0x1a;
    s.prir=3;s.prina=0x0600;s.prinb=0x0700;
    s.priority[0]=0x0204;s.priority[1]=0x0407;
    s.ratio[0]=0x0010;s.rbgRatio=0x10;s.specialColor=0x8;
    assert(lagi::rbgLayerCompositeSupported(s));
    assert(lagi::spriteUiPrioritySupported(s));
    const auto campLowSprite=lagi::spriteCompositeLookup(s,129+8);
    assert((campLowSprite&255u)==0u); // RBG0 p3 wins over sprite p2.
    assert(((campLowSprite>>8)&255u)==3u); // Add, not alpha.
    auto camp=s;
    camp.ccctl&=~0x100u;assert(!lagi::rbgLayerCompositeSupported(camp));
    camp=s;camp.prina=0x0300;assert(!lagi::rbgLayerCompositeSupported(camp));
    camp=s;camp.prinb=0x0500;assert(!lagi::rbgLayerCompositeSupported(camp));
    camp=s;camp.spctl=0x1723;assert(!lagi::rbgLayerCompositeSupported(camp));
    s=base;s.colorWindow=1;assert(!lagi::rbgLayerCompositeSupported(s));
    s=base;s.prinb=0x500;assert(!lagi::rbgLayerCompositeSupported(s));
    s=base;s.spctl=0x1323;s.ccctl=0x150;s.priority[0]=0x604;
    assert(lagi::rbgLayerCompositeSupported(s) && lagi::spriteUiPrioritySupported(s));
    unsigned checks=0;
    // Native tie-order sort is independent of the production shader's branch
    // structure. Enumerate every priority, presence combination and CCRR.
    for(unsigned sp=0;sp<8;++sp)for(unsigned rp=0;rp<8;++rp)for(unsigned np=0;np<8;++np)
    for(unsigned present=0;present<8;++present)for(unsigned ratio=0;ratio<32;++ratio)
    for(unsigned add=0;add<2;++add)for(unsigned selected=0;selected<2;++selected){
        unsigned colors[4]={((checks*37)^0x1234)&32767,((checks*61)^0x3721)&32767,
            ((checks*89)^0x5a7e)&32767,0x31f3}; // sprite, RBG0, NBG0, BACK
        samples[0]=float4(color(colors[0]),(present&1)?1.f:0.f);
        samples[1]=float4(color(colors[1]),(present&2)?1.f:0.f);
        samples[2]=float4(0,float(selected?(add?3:2):1)/255,float(ratio)/255,float(sp)/255);
        samples[3]=float4(color(colors[2]),(present&4)?1.f:0.f);
        struct Layer{unsigned key,id;};
        Layer layers[4]={{(present&1)&&sp?sp*8+5:0,0},
            {(present&2)&&rp?rp*8+4:0,1},{(present&4)&&np?np*8+3:0,2},{1,3}};
        std::sort(layers,layers+4,[](Layer a,Layer b){return a.key>b.key;});
        unsigned expected=colors[layers[0].id];
        if(layers[0].id==1 || (layers[0].id==0 && selected)) {
            const unsigned second=colors[layers[1].id];
            if(add){expected=0;for(unsigned shift=0;shift<15;shift+=5)
                expected|=std::min(((colors[layers[0].id]>>shift)&31)+((second>>shift)&31),31u)<<shift;}
            else expected=lagi::spriteRatioRgb555(expected,second,ratio)&32767;
        }
        const auto result=shade({0.5f,0.5f},0,1,2,3,{1,1,1,float(rp)},
            float4(color(colors[3]),1),{float(np),float(ratio),float(add),0});
        assert(word(result.rgb)==expected);assert(result.a==1);
        ++checks;
    }
    std::printf("PASS %u production-shader host cases: priorities/ties, holes, next raw source, CCRR; GPU pixels pending\n",checks);
}

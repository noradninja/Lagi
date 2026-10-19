#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
using s32=int32_t; using s16=int16_t; using u16=uint16_t;
using sSaturnPtr=int;
struct sVec3_FP { int value; };
using quadColor=std::array<u16,4>;
struct sLaserBerserkTask {
    struct sBeamTrail {
        sVec3_FP* m0_pSegments;
        s32 m4_numSegments, m8_activeCount;
        sSaturnPtr m10_config;
        u16 m14_vdp1Param;
    };
};
static std::array<uint8_t,1024> data;
static u16 readSaturnU16(int a) { return (data.at(a)<<8)|data.at(a+1); }
static s16 readSaturnS16(int a) { return static_cast<s16>(readSaturnU16(a)); }
static s32 readSaturnS32(int a) { return (readSaturnU16(a)<<16)|readSaturnU16(a+2); }
static int readSaturnEA(int a) { return readSaturnS32(a); }
static s32 setDividend(s32 width,s32 frame,s32 maximum) { return width*frame/maximum; }
static int calls;
static u16 expectedBase;
static void displayRaySegment(std::array<sVec3_FP,2>& points,s32 width,
    u16 address,s16 size,u16 palette,const quadColor* color,s32 mode)
{
    const int segment=2-calls;
    assert(points[0].value==segment+1 && points[1].value==segment);
    assert(width==(segment+1)*50);
    assert(address==static_cast<u16>(expectedBase+0x20));
    assert(palette==static_cast<u16>(expectedBase+0x40));
    assert(size==0x208 && mode==8);
    for(int i=0;i<4;++i) assert((*color)[i]==0x8000+segment*4+i);
    ++calls;
}
#include "laser_draw.inc"
static void put16(int a,u16 v) { data[a]=v>>8;data[a+1]=v; }
static void put32(int a,s32 v) { put16(a,v>>16);put16(a+2,v); }
int main()
{
    constexpr int config=16,widths=128,colors=256;
    put16(config+4,0x20); put16(config+6,0x208);put16(config+8,0x40);
    put32(config+12,widths);put32(config+16,colors);
    for(int i=0;i<3;++i) {
        put32(widths+i*4,(i+1)*100);
        for(int j=0;j<4;++j) put16(colors+i*8+j*2,0x8000+i*4+j);
    }
    sVec3_FP points[4]={{0},{1},{2},{3}};
    for(u16 base: {u16(0),u16(0x1234),u16(0xfff0)}) {
        expectedBase=base;
        sLaserBerserkTask::sBeamTrail trail{points,4,0,config,base};
        calls=0;renderBeamTrailSegments(&trail,15,30);assert(calls==3);
        trail.m8_activeCount=2;
        calls=0;renderBeamTrailSegments(&trail,15,30);assert(calls==1);
        trail.m8_activeCount=3;
        calls=0;renderBeamTrailSegments(&trail,15,30);assert(calls==0);
    }
    puts("PASS generated Chain/Prism trail relocation, segment order, width, Gouraud and lifetime bounds");
}

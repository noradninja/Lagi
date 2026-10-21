#include "lagi/scene_nbg0.h"
#include <array>
#include <cassert>
#include <cmath>
#include <algorithm>
#include <cstdio>
struct float2 {float x,y;float2(float a,float b):x(a),y(b){};};
struct float3 {float x,y,z;float3(float a,float b,float c):x(a),y(b),z(c){};};
struct float4 {float x,y,z,w;float4(float a=0,float b=0,float c=0,float d=0):x(a),y(b),z(c),w(d){};float2 xy()const{return {x,y};}};
float2 operator*(float2 a,float2 b){return {a.x*b.x,a.y*b.y};}
float2 operator+(float2 a,float2 b){return {a.x+b.x,a.y+b.y};}
float2 operator/(float2 a,float b){return {a.x/b,a.y/b};}
float2 floor(float2 a){return {std::floor(a.x),std::floor(a.y)};}
float2 min(float2 a,float2 b){return {std::min(a.x,b.x),std::min(a.y,b.y)};}
using sampler2D=const unsigned char*;
static unsigned packedReads=0,halfReads=0;
float4 rawTexel(sampler2D b,float i){++packedReads;unsigned a=unsigned(i)*4;return {b[a]/255.0f,b[a+1]/255.0f,b[a+2]/255.0f,b[a+3]/255.0f};}
float readByte(sampler2D b,float a){assert(a>=0&&a<0x81000);return b[unsigned(a)];}
float readBE16(sampler2D b,float a){++halfReads;return readByte(b,a)*256+readByte(b,a+1);}
float4 rgb555(float c){unsigned v=unsigned(c);return {float(v&31),float((v>>5)&31),float((v>>10)&31),1};}
#define uniform
#include "scene_shader.inc"
#undef uniform
static std::array<unsigned char,0x81000> memory{};
void word(unsigned a,unsigned v){memory[a]=v>>8;memory[a+1]=v;}
void longword(unsigned a,unsigned v){word(a,v>>16);word(a+2,v);}
int main(){
    // Exercise the production aligned path and compare all signed high-word
    // values against the old two-halfword formula, including fractional bits.
    for(unsigned hi=0;hi<65536;++hi){
        unsigned lo=(hi*40503u+17u)&65535u;
        longword(0x24000,(hi<<16)|lo);
        packedReads=halfReads=0;
        float got=nbgFixed(memory.data(),0x24000);
        float expected=float(hi>=32768?int(hi)-65536:int(hi))+lo/65536.0f;
        assert(got==expected && packedReads==1 && halfReads==0);
    }
    longword(0x24002,0xfff88000);
    assert(nbgFixed(memory.data(),0x24002)==-7.5f); // Unaligned fallback.
    longword(0,0x00018000);
    assert(nbgFixed(memory.data(),524288)==1.5f); // Wrapped aligned address.
    memory.fill(0);
    assert(lagi::sceneNbg0Supported(1,0x8000,0x101,0));
    assert(lagi::sceneNbg0Supported(1,0x8000,0,0));
    assert(!lagi::sceneNbg0Supported(1,0x8000,1,0)); // Not a fake ratio-to-add substitute.
    assert(!lagi::sceneNbg0Supported(0x11,0x8000,0x101,0));
    assert(!lagi::sceneNbg0Supported(1,0xc000,0x101,0));
    assert(lagi::sceneNbg0MapBase(0,63,3)==0x1e000);
    assert(lagi::sceneNbg0OverlayBand(0x1b,0x605,0x404)); // Native E011 priorities.
    assert(!lagi::sceneNbg0OverlayBand(0x1b,0x603,0x404));
    assert(!lagi::sceneNbg0OverlayBand(0x1b,0x607,0x404));
    float4 s[8];s[1]={0x1f000,0x1f000,0x1f000,0x1f000};
    s[2]={0,0,1,1};s[3]={0x24000,14,1,0};s[7]={512,512,5,0};
    for(unsigned i=0;i<1024;++i)word(0x1f000+i*2,0x400); // Horizontal flip.
    // First character's four 8x8 cells carry distinct palette entries.
    for(unsigned cell=0;cell<4;++cell)for(unsigned i=0;i<32;++i)memory[cell*32+i]=(cell+1)*0x11;
    for(unsigned c=1;c<=4;++c)word(0x80000+c*2,c);
    for(unsigned y=0;y<224;++y){longword(0x24000+y*12,0);longword(0x24004+y*12,y<<16);longword(0x24008+y*12,65536);}
    const auto sample=[&](int x,int y){return sampleSceneNbg0(memory.data(),float2((x+.5f)/352,(y+.5f)/224),s);};
    assert(sample(0,0).x==2);assert(sample(8,0).x==1);
    assert(sample(0,8).x==4);assert(sample(8,8).x==3);
    for(int y=0;y<224;++y)assert(sample(0,y).x==((y&8)?4:2));
    s[0].x=128;word(0x80000+(1024+2)*2,31);assert(sample(0,0).x==31);s[0].x=0;
    s[3].z=2;assert(sample(0,8).x==2);
    longword(0x24004+4*12,7*65536);assert(sample(0,9).x==4);
    longword(0x24004+4*12,4*65536);s[3].z=1;
    longword(0x24000,0xfff80000);assert(sample(0,0).x==1); // Negative X line-scroll wraps.
    longword(0x24008,2*65536);assert(sample(4,0).x==2);
    longword(0x24008,65536);
    s[6]={3,0,0,0};s[4]={4,0,20,223};
    assert(sample(0,0).w==0);assert(sample(8,0).w==1);
    s[6]={15,0,0,0};s[5]={16,0,28,223};
    assert(sample(8,0).w==1);assert(sample(24,0).w==1);
    s[6].x=143;assert(sample(8,0).w==0);assert(sample(16,0).w==1);assert(sample(24,0).w==0);
    word(0x30000,16);word(0x30002,40);s[6]={3,0x30000,0,1};
    assert(sample(0,0).w==0);assert(sample(8,0).w==1);
    s[6]={0,0,0,0};memory.fill(0);
    assert(sample(0,0).w==0);s[0].z=1;assert(sample(0,0).w==1);
    puts("PASS production NBG0 shader scalar addressing, flips, signed line scroll, windows and transparent zero; not GPU execution");
}

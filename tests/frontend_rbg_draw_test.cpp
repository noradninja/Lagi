#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <vector>
#include <array>
#include "../include/lagi/rbg_window_spans.h"
namespace azel {struct DebugTextureVertex {float x,y,z,u,v;};}
namespace lagi {static bool rbgTile16EightBit(unsigned,unsigned){return false;}}
namespace logging {template<class... T>static void writef(const char*,T...){} }
struct RbgGpuState {
    float parameterFormat[2]{},planesA[16]{},planesB[16]{},ctrl[16]{};
    float plsz=0,format[4]{},transformA[8]{},transformB[8]{};
    float coefficientA[4]{},coefficientB[4]{};
};
struct ViewerMat4 {float m[16]{};};
static ViewerMat4 viewerIdentity(){return {};}
static bool g_sceneVdp2ByteTextureValid=false,g_rbgDecodedPagesReady=false;
static bool g_frontendRbgDecodedReady=false;
static bool g_rbgTileBound=false,g_rbgDecodedBound=false;
static bool g_rbgRowsBound=false,g_rbgCoefficientRowsReady=false;
static int g_rbgCoefficientRowsTexture=0;
static float g_rbgDecodedPlanesA[16]{},g_rbgDecodedPlanesB[16]{};
static int g_sceneVdp2ByteTexture=0,g_rbgDecodedAtlasTexture=0;
static void* g_probeContext=nullptr;
static int g_textureVertexProgram=0,g_textureWvpParam=0;
static int g_vdp2Rbg0PlaneParam[4]{},g_vdp2Rbg0BParam[9]{};
static int g_vdp2Rbg0Transform0Param=0,g_vdp2Rbg0Transform1Param=0;
static int g_vdp2Rbg0CoefficientParam=0,g_vdp2Rbg0InfoParam=0;
static int g_vdp2Rbg0FormatParam=0,g_vdp2Rbg0LayerWindowParam=0;
static int g_vdp2Rbg0LayerWindowInfoParam=0,g_vdp2Rbg0SwitchCoefficientParam=0;
static int g_vdp2Rbg0SwitchInfoParam=0;
static azel::DebugTextureVertex coverage[lagi::RbgWindowMaxSpans*2*4];
static std::uint16_t coverageIndices[lagi::RbgWindowMaxSpans*2*6];
static azel::DebugTextureVertex* g_vdp2WindowVertices=coverage;
static std::uint16_t* g_vdp2WindowIndices=coverageIndices;
static void selectRbgTileProgram(bool,bool,bool){}
static void sceGxmSetFragmentTexture(void*,int,const int*){}
static int sceGxmReserveFragmentDefaultUniformBuffer(void*,void** p){*p=coverage;return 0;}
static int sceGxmReserveVertexDefaultUniformBuffer(void*,void** p){*p=coverage;return 0;}
static void sceGxmSetUniformDataF(void*,int,int,int,const float*){}
static void sceGxmSetVertexProgram(void*,int){}
static void drawAzelColorOffsetForLayer(unsigned,bool){}
enum {SCE_GXM_PRIMITIVE_TRIANGLES,SCE_GXM_INDEX_FORMAT_U16};
static const azel::DebugTextureVertex* stream=nullptr;
struct Draw {std::vector<azel::DebugTextureVertex> quads;unsigned indices;};
static std::vector<Draw> draws;
static int sceGxmSetVertexStream(void*,int,const azel::DebugTextureVertex* v){stream=v;return 0;}
static int sceGxmDraw(void*,int,int,const std::uint16_t* indices,unsigned n) {
    Draw capture{{},n};
    for(unsigned q=0;q<n/6;++q)for(unsigned v=0;v<4;++v)
        capture.quads.push_back(stream[indices[q*6]+v]);
    draws.push_back(std::move(capture));return 0;
}
// Actual production draw function extracted by the runner, not a replica.
#include "frontend_rbg_draw.inc"
static unsigned char raw[0x81000]{};
static void put16(unsigned addr,unsigned v){raw[addr]=v&255;raw[addr+1]=v>>8;}
int main() {
    RbgGpuState r{};
    r.ctrl[0]=3;r.ctrl[1]=0x101;r.ctrl[2]=0x100;
    r.ctrl[4]=12;r.ctrl[6]=0x3c000;r.ctrl[7]=2;
    r.ctrl[12]=0;r.ctrl[13]=0;r.ctrl[14]=352;r.ctrl[15]=224;
    for(unsigned y=0;y<224;++y){put16(0x3c000+y*4,0);put16(0x3c002+y*4,y<112?0x2c0:0xffff);}
    const azel::DebugTextureVertex quad[4]={{-1,1,.5f,0,0},{1,1,.5f,1,0},{-1,-1,.5f,0,1},{1,-1,.5f,1,1}};
    const std::uint16_t indices[6]={0,1,2,2,1,3};
    for(bool frontend:{true,false}) {
        draws.clear();
        assert(drawVdp2Rbg0Gpu(r,raw,quad,indices,1,frontend));
        assert(draws.size()==2 && draws[0].indices==6 && draws[1].indices==6);
        assert(draws[0].quads[0].v==.5f && draws[0].quads[2].v==1);
        assert(draws[1].quads[0].v==0 && draws[1].quads[2].v==.5f);
        for(const auto& d:draws)assert(d.quads[0].u==0 && d.quads[1].u==1);
    }
    // No coverage storage must fail before either draw, never fullscreen A.
    draws.clear();g_vdp2WindowVertices=nullptr;
    assert(!drawVdp2Rbg0Gpu(r,raw,quad,indices,1,true) && draws.empty());
    g_vdp2WindowVertices=coverage;
    // Entirely disabled inside-window: B only, selected geometry covers all rows.
    for(unsigned y=0;y<224;++y)put16(0x3c002+y*4,0xffff);
    draws.clear();assert(drawVdp2Rbg0Gpu(r,raw,quad,indices,1,true));
    assert(draws.size()==1 && draws[0].indices==6);
    // Disabled outside-window: A only, exactly one fullscreen rectangle.
    r.ctrl[4]=8;draws.clear();assert(drawVdp2Rbg0Gpu(r,raw,quad,indices,1,true));
    assert(draws.size()==1 && draws[0].indices==6);
    assert(draws[0].quads[0].u==0 && draws[0].quads[1].u==1);
    // Exercise the actual mapped-vertex extension on moving/split windows.
    // Rasterized rectangles must equal every original scanline's ownership.
    for(unsigned test=0;test<128;++test) {
        r.ctrl[4]=float((test&15)|((test&16)<<3));r.ctrl[7]=float((test>>5)&3);
        r.ctrl[5]=0x3b000;
        r.ctrl[8]=50;r.ctrl[10]=170;r.ctrl[9]=test&1?20:0;r.ctrl[11]=200;
        r.ctrl[12]=150;r.ctrl[14]=300;r.ctrl[13]=test&2?40:0;r.ctrl[15]=223;
        for(unsigned y=0;y<224;++y)for(unsigned w=0;w<2;++w) {
            const unsigned a=unsigned(r.ctrl[5+w])+y*4;
            put16(a,((y/7*11+w*90+test)%352)*2);
            put16(a+2,(y%13==0)?0xffff:((y/7*17+w*150+test)%352)*2);
        }
        std::array<unsigned char,352*224> expected{},actual;actual.fill(255);
        unsigned count[2]{};
        assert(lagi::emitRbgWindowSpans(r.ctrl,raw,[&](unsigned p,int y,int first,int last){
            ++count[p];for(int x=first;x<=last;++x)expected[y*352+x]=static_cast<unsigned char>(p);
        }));
        draws.clear();const float extent=test&1?.75f:1.0f;
        assert(drawVdp2Rbg0Gpu(r,raw,quad,indices,extent,(test&1)!=0));
        unsigned next=0;
        for(unsigned p:{1u,0u})if(count[p]) {
            const auto& d=draws[next++];assert(d.indices/6<=count[p]);
            for(unsigned q=0;q<d.quads.size();q+=4) {
                const auto& top=d.quads[q];const auto& right=d.quads[q+1];const auto& bottom=d.quads[q+2];
                assert(std::abs(top.x-(-extent+2*extent*top.u))<1e-6f);
                assert(std::abs(bottom.y-(1-2*bottom.v))<1e-6f);
                const int x0=int(std::round(top.u*352)),x1=int(std::round(right.u*352));
                const int y0=int(std::round(top.v*224)),y1=int(std::round(bottom.v*224));
                for(int y=y0;y<y1;++y)for(int x=x0;x<x1;++x) {
                    const float u=top.u+(float(x-x0)+.5f)/float(x1-x0)*(right.u-top.u);
                    const float v=top.v+(float(y-y0)+.5f)/float(y1-y0)*(bottom.v-top.v);
                    assert(int(std::floor(u*352))==x && int(std::floor(v*224))==y);
                    assert(actual[y*352+x]==255);actual[y*352+x]=static_cast<unsigned char>(p);
                }
            }
        }
        assert(next==draws.size() && expected==actual);
    }
    std::puts("PASS actual frontend/native RBG draw: two D5 rectangles, exact split/moving coverage, empty windows, missing-buffer failure");
}

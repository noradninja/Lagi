#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <vector>
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
static azel::DebugTextureVertex coverage[224*3*4];
static std::uint16_t coverageIndices[224*3*6];
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
static int sceGxmDraw(void*,int,int,const std::uint16_t*,unsigned n) {
    draws.push_back({{stream,stream+n/6*4},n});return 0;
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
        assert(draws.size()==2 && draws[0].indices==6 && draws[1].indices==112*6);
        for(unsigned y=0;y<112;++y) {
            auto& q=draws[1].quads;
            assert(q[y*4].u==0 && q[y*4+1].u==1);
            assert(std::abs(q[y*4].v-float(y)/224)<1e-6f);
            assert(std::abs(q[y*4+2].v-float(y+1)/224)<1e-6f);
        }
    }
    // No coverage storage must fail before either draw, never fullscreen A.
    draws.clear();g_vdp2WindowVertices=nullptr;
    assert(!drawVdp2Rbg0Gpu(r,raw,quad,indices,1,true) && draws.empty());
    g_vdp2WindowVertices=coverage;
    // Entirely disabled inside-window: B only.
    for(unsigned y=0;y<224;++y)put16(0x3c002+y*4,0xffff);
    draws.clear();assert(drawVdp2Rbg0Gpu(r,raw,quad,indices,1,true));
    assert(draws.size()==1 && draws[0].indices==6);
    // Disabled outside-window: adjacent spans cover every pixel, no overlap.
    r.ctrl[4]=8;draws.clear();assert(drawVdp2Rbg0Gpu(r,raw,quad,indices,1,true));
    assert(draws.size()==2 && draws[1].indices==224*2*6);
    for(unsigned y=0;y<224;++y) {
        const auto& q=draws[1].quads;
        assert(q[y*8].u==0 && q[y*8+1].u==q[y*8+4].u && q[y*8+5].u==1);
    }
    std::puts("PASS actual frontend/native RBG draw: B base, exact 112-row A coverage, empty windows, missing-buffer failure");
}

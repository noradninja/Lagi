#include <cassert>
#include <cstdio>
#include <algorithm>
#include <array>
struct Vertex {float x,y,z,u,v;};
struct ViewerMat4 {float m[16]{};};
static ViewerMat4 viewerIdentity(){ViewerMat4 m;for(int i=0;i<4;++i)m.m[i*5]=1;return m;}
static void* g_probeContext=nullptr;
static int g_textureVertexProgram=1,g_textureWvpParam=2,g_movieTexture=3;
static int g_vdp2NbgFragmentProgram=4,g_vdp2InfoParam=5;
static Vertex mapped[8]{};static Vertex* g_movieVertices=mapped;
static unsigned short indices[6]={0,1,2,2,1,3};static auto* g_movieIndices=indices;
static float g_movieVdp2Info[4]{};
static unsigned calls=0,draws=0;
static bool vertexFail=false,fragmentFail=false,streamFail=false,drawFail=false;
static std::array<float,4> published{};
enum {SCE_GXM_PRIMITIVE_TRIANGLES,SCE_GXM_INDEX_FORMAT_U16};
static void sceGxmSetVertexProgram(void*,int program){++calls;assert(program==1);}
static int sceGxmReserveVertexDefaultUniformBuffer(void*,void** p){++calls;*p=mapped;return vertexFail?-1:0;}
static int sceGxmReserveFragmentDefaultUniformBuffer(void*,void** p){++calls;*p=mapped;return fragmentFail?-1:0;}
static void sceGxmSetUniformDataF(void*,int parameter,int,unsigned count,const float* values){
    ++calls;
    if(parameter==2){assert(count==16);for(unsigned i=0;i<16;++i)assert(values[i]==float(i%5==0));}
    else {assert(parameter==5 && count==4);std::copy_n(values,4,published.begin());}
}
static void sceGxmSetFragmentTexture(void*,unsigned unit,const int* texture){++calls;assert(unit==0 && texture==&g_movieTexture);}
static void sceGxmSetFragmentProgram(void*,int program){++calls;assert(program==4);}
static int sceGxmSetVertexStream(void*,unsigned stream,const Vertex* v){++calls;assert(stream==0 && v==mapped+4);return streamFail?-1:0;}
static int sceGxmDraw(void*,int primitive,int format,const unsigned short* i,unsigned n){
    ++calls;++draws;assert(primitive==SCE_GXM_PRIMITIVE_TRIANGLES && format==SCE_GXM_INDEX_FORMAT_U16 && i==indices && n==6);
    return drawFail?-1:0;
}
#include "../src/platform/vita/neptune_frontend_nbg.inc"
int main() {
    assert(drawFrontendNbgNative() && !calls);
    g_movieVdp2Info[0]=2;assert(drawFrontendNbgNative() && !calls); // BGON off.
    g_movieVdp2Info[3]=1;g_movieVdp2Info[1]=-123;g_movieVdp2Info[2]=511;
    assert(drawFrontendNbgNative() && draws==1);
    for(unsigned i=0;i<4;++i)assert(published[i]==g_movieVdp2Info[i]);
    vertexFail=true;assert(!drawFrontendNbgNative());vertexFail=false;
    fragmentFail=true;assert(!drawFrontendNbgNative());fragmentFail=false;
    streamFail=true;assert(!drawFrontendNbgNative());streamFail=false;
    drawFail=true;assert(!drawFrontendNbgNative());drawFail=false;
    const unsigned before=calls;g_movieVdp2Info[0]=0;
    assert(drawFrontendNbgNative() && calls==before); // Title remains separate.
    std::puts("PASS production native NBG submission: live visibility/scroll, raw binding, native quad, title exclusion and submission failures");
}

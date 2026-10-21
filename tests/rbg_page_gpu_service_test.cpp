#include "../include/lagi/rbg_decoded_page.h"
#include "../include/lagi/rbg_format.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <cstdio>
#define LAGI_RBG_DECODED_PAGES 1
using SceUID=int;
struct SceGxmTexture {void* data=nullptr;};
static constexpr int SCE_GXM_MEMORY_ATTRIB_READ=1,SCE_GXM_TEXTURE_FORMAT_A8B8G8R8=0,SCE_GXM_TEXTURE_FILTER_POINT=0;
static constexpr unsigned kSceneVdp2BlockCount=129;
static bool allocationFails=false;static unsigned allocations=0;
static void* probeCdramAlloc(unsigned bytes,unsigned,SceUID* uid){++allocations;if(allocationFails)return nullptr;*uid=1;return std::calloc(1,bytes);}
static void* probeGpuAlloc(unsigned bytes,unsigned flags,SceUID* uid){return probeCdramAlloc(bytes,flags,uid);}
static void freeMovieMappedBlock(SceUID& uid,void*& p){std::free(p);p=nullptr;uid=-1;}
static int sceGxmTextureInitLinear(SceGxmTexture* t,void* p,int,unsigned,unsigned,unsigned){t->data=p;return p?0:-1;}
static void sceGxmTextureSetMinFilter(SceGxmTexture*,int){}
static void sceGxmTextureSetMagFilter(SceGxmTexture*,int){}
static unsigned long long sceKernelGetProcessTimeWide(){static unsigned long long time=0;return ++time;}
namespace logging {template<class... T>void writef(const char*,T...){};}
static bool g_rbgDecodedPagesReady=false;
static SceGxmTexture g_rbgDecodedAtlasTexture{};
static float g_rbgDecodedPlanesA[16]{},g_rbgDecodedPlanesB[16]{};
static struct {float format[4]{},planesA[16]{},planesB[16]{},ctrl[16]{},parameterFormat[2]{};} g_sceneRbg0;
static std::array<std::uint8_t,0x81000> image{};
static const std::uint8_t* g_sceneVdp2Raw=image.data();
static std::uint32_t g_sceneVdp2BlockGenerations[129]{};
#include "../src/platform/vita/neptune_rbg_decoded_pages.inc"
static void planes(float* p,unsigned base){std::fill_n(p,16,float(base));}
static unsigned slot(unsigned base){return g_rbgPageLookup[base/2048];}
int main() {
    for(unsigned i=0;i<image.size();++i)image[i]=std::uint8_t(i*17);
    for(unsigned a=0x70000;a<0x72800;a+=2){image[a]=0;image[a+1]=0;}
    std::fill_n(g_sceneVdp2BlockGenerations,129,1u);
    g_sceneRbg0.format[0]=0x1100;g_sceneRbg0.format[1]=0x8000;
    planes(g_sceneRbg0.planesA,0x70000);planes(g_sceneRbg0.planesB,0x70800);
    g_sceneRbg0.ctrl[0]=2;
    assert(prepareRbgDecodedPages());assert(slot(0x70000)!=slot(0x70800));
    assert(unsigned(g_rbgDecodedPlanesA[0])==slot(0x70000));
    assert(unsigned(g_rbgDecodedPlanesB[0])==slot(0x70800));
    const auto original=slot(0x70000);
    auto* atlas=static_cast<std::uint32_t*>(g_rbgAtlasPixels);
    atlas[0]=0x12345678; // Cache hits must not overwrite existing atlas storage.
    ++g_sceneVdp2BlockGenerations[126];
    assert(prepareRbgDecodedPages());assert(atlas[0]==0x12345678);
    ++g_sceneVdp2BlockGenerations[128];
    assert(prepareRbgDecodedPages());assert(atlas[0]==0x12345678); // Palette stays live on GPU.
    g_sceneRbg0.format[2]=7;assert(prepareRbgDecodedPages());assert(atlas[0]==0x12345678);
    ++g_sceneVdp2BlockGenerations[0];
    assert(prepareRbgDecodedPages());assert(atlas[0]!=0x12345678);
    // Encounter new C before surviving A: reserve A first, do not evict it.
    planes(g_sceneRbg0.planesA,0x71000);g_sceneRbg0.planesA[1]=float(0x70000);
    g_sceneRbg0.ctrl[0]=0;
    assert(prepareRbgDecodedPages());assert(slot(0x70000)==original);
    // Four pages fit; a fifth must reject the entire cache route.
    for(unsigned i=0;i<4;++i)g_sceneRbg0.planesA[i]=float(0x70000+i*2048);
    assert(prepareRbgDecodedPages());
    g_sceneRbg0.planesA[4]=float(0x72000);
    assert(!prepareRbgDecodedPages());
    planes(g_sceneRbg0.planesA,0x70000);
    g_sceneRbg0.format[1]=0x801f;assert(!prepareRbgDecodedPages());
    g_sceneRbg0.format[1]=0x8000;assert(prepareRbgDecodedPages());
    g_sceneRbg0.parameterFormat[0]=4;assert(!prepareRbgDecodedPages());
    g_sceneRbg0.parameterFormat[0]=0;assert(prepareRbgDecodedPages());
    g_sceneRbg0.parameterFormat[0]=3; // Four 512x512 pages per plane.
    assert(prepareRbgDecodedPages());
    for(unsigned page=0;page<4;++page)
        assert(((unsigned(g_rbgDecodedPlanesA[0])>>(page*2))&3)==slot(0x70000+page*2048));
    g_sceneRbg0.parameterFormat[0]=0;
    releaseRbgDecodedPages();
    assert(!g_rbgAtlasPixels && !g_rbgPageScratch && !g_rbgDecodedPagesReady);
    allocationFails=true;assert(!prepareRbgDecodedPages());
    const auto attempts=allocations;assert(!prepareRbgDecodedPages());assert(allocations==attempts);
    releaseRbgDecodedPages();
    std::puts("PASS production page service hits, uploads, slot reservation, capacity, unsupported and allocation fallback");
}

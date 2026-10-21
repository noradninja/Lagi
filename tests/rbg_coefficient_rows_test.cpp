#include "../include/lagi/rbg_coefficient_rows.h"
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <limits>
static std::array<unsigned char,0x81000> raw{};
using s32=std::int32_t;using u32=std::uint32_t;
static void* getVdp2Vram(unsigned a){return raw.data()+a;}
// Actual upstream 32-bit decoder, extracted by the runner.
#include "native_coefficient_reference.inc"
static void word(unsigned address,unsigned value,unsigned bytes=4){
    for(unsigned b=0;b<bytes;++b)raw[(address+b)&0x7ffff]=static_cast<unsigned char>(value>>(b*8));
}
#define LAGI_RBG_COEFFICIENT_ROWS 1
using SceUID=int;
struct SceGxmTexture {void* data=nullptr;};
static constexpr unsigned SCE_GXM_MEMORY_ATTRIB_READ=1,SCE_GXM_TEXTURE_FORMAT_F32F32_GR=2,SCE_GXM_TEXTURE_FILTER_POINT=0;
static bool allocationFails=false,descriptorFails=false;
static unsigned allocations=0;
static void* probeGpuAlloc(unsigned size,unsigned,SceUID* uid){++allocations;if(allocationFails)return nullptr;*uid=1;return std::calloc(1,size);}
static void freeMovieMappedBlock(SceUID& uid,void*& p){std::free(p);p=nullptr;uid=-1;}
static int sceGxmTextureInitLinear(SceGxmTexture* t,void* p,unsigned format,unsigned w,unsigned h,unsigned){
    assert(format==SCE_GXM_TEXTURE_FORMAT_F32F32_GR && w==256 && h==2);t->data=p;return descriptorFails?-1:0;
}
static void sceGxmTextureSetMinFilter(SceGxmTexture*,unsigned){}
static void sceGxmTextureSetMagFilter(SceGxmTexture*,unsigned){}
static unsigned long long sceKernelGetProcessTimeWide(){static unsigned long long t=0;return ++t;}
namespace logging {template<class... T>void writef(const char*,T...){} }
static bool g_rbgDecodedPagesReady=true,g_rbgCoefficientRowsReady=false;
static SceGxmTexture g_rbgCoefficientRowsTexture{};
static struct {float ctrl[16]{},coefficientA[4]{},coefficientB[4]{};} g_sceneRbg0;
static const unsigned char* g_sceneVdp2Raw=raw.data();
#include "../src/platform/vita/neptune_rbg_coefficient_rows.inc"
int main() {
    for(unsigned value=0;value<65536;++value) {
        word(0,value,2);
        auto c=lagi::decodeRbgCoefficient(raw.data(),0,2);
        int signedValue=int(value&0x7fff);if(signedValue>=16384)signedValue-=32768;
        assert(c.scale==float(signedValue)/1024 && c.invalid==float(value>>15));
    }
    // Exhaust every 24-bit payload against the native function, with varied
    // upper flags, so sign extension and invalid-bit behavior are not guessed.
    for(unsigned value=0;value<0x1000000;++value) {
        word(0,value|((value&0xff)<<24));
        bool invalid=false;const s32 reference=readCoefficientFromVRAM(0,invalid);
        auto c=lagi::decodeRbgCoefficient(raw.data(),0,4);
        assert(c.scale==float(reference)/65536 && c.invalid==float(invalid));
    }
    for(unsigned address:{0u,2u,0x7fffeu,0x7ffffu}) {
        word(address,0x80800001);
        auto c=lagi::decodeRbgCoefficient(raw.data(),address,4);
        assert(c.scale==float(-8388607)/65536 && c.invalid==1);
    }
    for(unsigned i=0;i<0x80000;i+=4)word(i,0x80000000|i);
    for(float step:{-32768.0f,-176.0f,-0.5f,0.0f,0.5f,176.0f,32768.0f}) {
        float coefficient[4]={32768,step,0,0};lagi::RbgCoefficientRows rows;
        assert(lagi::buildRbgCoefficientRows(raw.data(),coefficient,0x40000,4,true,rows));
        for(unsigned y=0;y<224;++y) {
            const auto index=static_cast<unsigned>(static_cast<std::int64_t>(std::floor(32768.0+double(step)*y)))&65535;
            auto expected=lagi::decodeRbgCoefficient(raw.data(),0x40000+index*4,4);
            assert(rows[y].scale==expected.scale && rows[y].invalid==expected.invalid);
        }
    }
    float perPixel[4]={0,0,.5f,0};lagi::RbgCoefficientRows output;
    output.fill({17,1});
    assert(!lagi::buildRbgCoefficientRows(raw.data(),perPixel,0,4,true,output));
    assert(output[0].scale==17); // Rejection leaves output unchanged.
    assert(lagi::buildRbgCoefficientRows(nullptr,perPixel,0,4,false,output));
    for(auto c:output)assert(c.scale==1 && c.invalid==0);
    float boundary[4]={32768.99609375f,1.0f/1024,0,0};
    assert(!lagi::buildRbgCoefficientRows(raw.data(),boundary,0,4,true,output));
    boundary[0]=std::numeric_limits<float>::quiet_NaN();
    assert(!lagi::buildRbgCoefficientRows(raw.data(),boundary,0,4,true,output));
    // Execute the production service: upload current A/B rows, never reuse
    // stale contents after a register-only or coefficient-word change.
    g_sceneRbg0.ctrl[0]=2;g_sceneRbg0.ctrl[1]=0x101;
    g_sceneRbg0.coefficientA[1]=g_sceneRbg0.coefficientB[1]=1;
    assert(prepareRbgCoefficientRows() && allocations==1);
    auto* upload=static_cast<lagi::RbgCoefficientRow*>(g_rbgCoefficientRowsPixels);
    assert(upload[1].invalid==1 && upload[1].scale==4.0f/65536);
    word(4,0x20000);assert(prepareRbgCoefficientRows());
    assert(upload[1].scale==2 && upload[1].invalid==0);
    g_sceneRbg0.coefficientA[2]=.5f;assert(!prepareRbgCoefficientRows());
    g_sceneRbg0.ctrl[0]=1;assert(prepareRbgCoefficientRows()); // A unused.
    assert(upload[1].scale==1 && upload[1].invalid==0);
    g_rbgDecodedPagesReady=false;assert(!prepareRbgCoefficientRows());g_rbgDecodedPagesReady=true;
    releaseRbgCoefficientRows();allocationFails=true;
    assert(!prepareRbgCoefficientRows());const auto count=allocations;
    assert(!prepareRbgCoefficientRows() && allocations==count);
    releaseRbgCoefficientRows();allocationFails=false;descriptorFails=true;
    assert(!prepareRbgCoefficientRows() && !g_rbgCoefficientRowsPixels);
    releaseRbgCoefficientRows();descriptorFails=false;assert(prepareRbgCoefficientRows());
    releaseRbgCoefficientRows();
    std::puts("PASS all 16-bit/24-bit payloads, native decoder oracle, wrapped row indices, precision rejection, live upload and allocation/descriptor fallback");
}

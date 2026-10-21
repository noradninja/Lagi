#include <array>
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstring>
struct SceGxmProgram {unsigned id;};
struct SceGxmProgramParameter {unsigned program,index;};
struct SceGxmFragmentProgram {unsigned id;};
using SceGxmShaderPatcherId=unsigned;
struct SceGxmBlendInfo {int colorFunc,alphaFunc,colorSrc,alphaSrc,colorDst,alphaDst,colorMask;};
enum {SCE_GXM_BLEND_FUNC_ADD=1,SCE_GXM_BLEND_FACTOR_SRC_ALPHA=2,
    SCE_GXM_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA=3,SCE_GXM_BLEND_FACTOR_ONE=4,
    SCE_GXM_COLOR_MASK_ALL=15,SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4=0,SCE_GXM_MULTISAMPLE_NONE=0};
alignas(4) static const unsigned char _binary_lagi_vdp2_rbg0_tile16_8bit_f_gxp_start[]={1,0,0,0};
alignas(4) static const unsigned char _binary_lagi_vdp2_rbg0_pages_f_gxp_start[]={2,0,0,0};
alignas(4) static const unsigned char _binary_lagi_texture_v_gxp_start[]={3,0,0,0};
static void* g_probeShaderPatcher=reinterpret_cast<void*>(1);
static void* g_probeContext=reinterpret_cast<void*>(1);
static SceGxmFragmentProgram fragments[3]={{0},{1},{2}};
static SceGxmFragmentProgram* g_vdp2Rbg0FragmentProgram=&fragments[0];
static SceGxmFragmentProgram* current=nullptr;
static SceGxmProgramParameter parameters[3][22]{};
static unsigned nextParameter[3]{},registrations[3]{},releases[3]{},unregisters[3]{};
static unsigned missingUniformProgram=0,failedCreationProgram=0;
static int sceGxmProgramCheck(const SceGxmProgram*){return 0;}
static const SceGxmProgramParameter* sceGxmProgramFindParameterByName(const SceGxmProgram* p,const char*) {
    unsigned i=nextParameter[p->id]++;
    assert(i<22);
    if(p->id==missingUniformProgram && i==7)return nullptr;
    parameters[p->id][i]={p->id,i};return &parameters[p->id][i];
}
static int sceGxmShaderPatcherRegisterProgram(void*,const SceGxmProgram* p,SceGxmShaderPatcherId* id){*id=p->id;++registrations[p->id];return 0;}
static int sceGxmShaderPatcherCreateFragmentProgram(void*,SceGxmShaderPatcherId id,int,int,
    const SceGxmBlendInfo* blend,const SceGxmProgram*,SceGxmFragmentProgram** result) {
    assert(blend->colorSrc==SCE_GXM_BLEND_FACTOR_SRC_ALPHA);
    assert(blend->alphaSrc==SCE_GXM_BLEND_FACTOR_ONE);
    assert(blend->colorDst==SCE_GXM_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
    if(id==failedCreationProgram)return -1;
    *result=&fragments[id];return 0;
}
static void sceGxmShaderPatcherReleaseFragmentProgram(void*,SceGxmFragmentProgram* p){++releases[p->id];}
static void sceGxmShaderPatcherUnregisterProgram(void*,SceGxmShaderPatcherId id){++unregisters[id];}
static void sceGxmSetFragmentProgram(void*,SceGxmFragmentProgram* p){current=p;}
namespace logging {template<class... T>static void writef(const char*,T...){};}
static const SceGxmProgramParameter *g_vdp2Rbg0PlaneParam[4]{},*g_vdp2Rbg0BParam[9]{};
static const SceGxmProgramParameter *g_vdp2Rbg0Transform0Param=nullptr,*g_vdp2Rbg0Transform1Param=nullptr,
    *g_vdp2Rbg0CoefficientParam=nullptr,*g_vdp2Rbg0InfoParam=nullptr,*g_vdp2Rbg0FormatParam=nullptr,
    *g_vdp2Rbg0SwitchCoefficientParam=nullptr,*g_vdp2Rbg0SwitchInfoParam=nullptr,
    *g_vdp2Rbg0LayerWindowParam=nullptr,*g_vdp2Rbg0LayerWindowInfoParam=nullptr;
#include "../src/platform/vita/neptune_rbg_tile_program.inc"
static auto slots() {
    return std::array<const SceGxmProgramParameter**,22>{
        &g_vdp2Rbg0PlaneParam[0],&g_vdp2Rbg0PlaneParam[1],&g_vdp2Rbg0PlaneParam[2],&g_vdp2Rbg0PlaneParam[3],
        &g_vdp2Rbg0BParam[0],&g_vdp2Rbg0BParam[1],&g_vdp2Rbg0BParam[2],&g_vdp2Rbg0BParam[3],
        &g_vdp2Rbg0BParam[4],&g_vdp2Rbg0BParam[5],&g_vdp2Rbg0BParam[6],&g_vdp2Rbg0BParam[7],&g_vdp2Rbg0BParam[8],
        &g_vdp2Rbg0Transform0Param,&g_vdp2Rbg0Transform1Param,&g_vdp2Rbg0CoefficientParam,
        &g_vdp2Rbg0InfoParam,&g_vdp2Rbg0FormatParam,&g_vdp2Rbg0SwitchCoefficientParam,&g_vdp2Rbg0SwitchInfoParam,
        &g_vdp2Rbg0LayerWindowParam,&g_vdp2Rbg0LayerWindowInfoParam};
}
static void initialize() {
    for(unsigned i=0;i<22;++i){parameters[0][i]={0,i};*slots()[i]=&parameters[0][i];}
    std::fill_n(nextParameter,3,0);missingUniformProgram=failedCreationProgram=0;
    current=nullptr;
}
static void check(unsigned id) {
    assert(current==&fragments[id]);
    for(unsigned i=0;i<22;++i)assert(*slots()[i]==&parameters[id][i]);
    assert(g_rbgTileBound==(id==1));assert(g_rbgDecodedBound==(id==2));
}
int main() {
    initialize();
    selectRbgTileProgram(false);check(0);
    selectRbgTileProgram(true);check(1);
    selectRbgTileProgram(true,true);check(2);
    selectRbgTileProgram(false);check(0); // Frontend or unsupported format.
    selectRbgTileProgram(true,true);check(2);
    selectRbgTileProgram(true);check(1);
    selectRbgTileProgram(false,true);check(0);
    assert(registrations[1]==1 && registrations[2]==1);
    releaseRbgTileProgram();
    assert(releases[1]==1 && releases[2]==1 && unregisters[1]==1 && unregisters[2]==1);
    initialize();missingUniformProgram=2;
    selectRbgTileProgram(true);check(1);
    selectRbgTileProgram(true,true);check(0); // Missing cache uniform: restore all generic slots.
    selectRbgTileProgram(true);check(1);
    releaseRbgTileProgram();
    initialize();failedCreationProgram=2;
    selectRbgTileProgram(true,true);check(0);
    const auto before=unregisters[2];
    releaseRbgTileProgram();assert(unregisters[2]==before+1);
    std::puts("PASS production RBG generic/byte/cache switching, reflection failure, creation failure and cleanup");
}

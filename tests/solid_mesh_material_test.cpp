#include "../include/lagi/vdp1_color_calculation.h"
#include <cmath>
#include <cassert>
#include <cstdio>
#include <initializer_list>
using std::floor;
static float frac(float value){return value-floor(value);}
#include "../shaders/vdp1_mesh_coverage.cginc"
using SceGxmFragmentProgram=unsigned;
static unsigned opaque=1,fade=2,mesh=3,half=4,halfMesh=5;
static auto* g_probeFragmentProgram=&opaque;
static auto* g_fadeFragmentProgram=&fade;
static auto* g_solidMeshFragmentProgram=&mesh;
enum {BlendSolid,BlendSolidMesh};
static SceGxmFragmentProgram* g_nativeBlendPrograms[]={&half,&halfMesh};
#include "../src/platform/vita/neptune_solid_material.inc"
int main(){
    for(unsigned mode=0;mode<65536;++mode){
        const bool isHalf=(mode&3)==3,isMesh=(mode&256)!=0;
        auto* expected=isMesh?(isHalf?&halfMesh:&mesh):
            isHalf?(LAGI_VDP2_SPRITE_COMPOSITE?&half:&fade):&opaque;
        assert(nativeSolidFragmentProgram(mode)==expected);
    }
    for(float phase:{0.25f,0.5f,0.75f}){
        unsigned discarded=0;
        for(int y=0;y<544;++y)for(int x=0;x<960;++x){
            const bool reject=lagiVdp1MeshDiscard(x+phase,y+phase);
            assert(reject==bool((x+y)&1));discarded+=reject;
        }
        assert(discarded==960*544/2);
    }
    for(int y=-8;y<8;++y)for(int x=-8;x<8;++x)
        assert(lagiVdp1MeshDiscard(x+0.5f,y+0.5f)==bool((x+y)&1));
    puts("PASS production solid material selection for every PMOD and native checkerboard coverage; device rendering pending");
}

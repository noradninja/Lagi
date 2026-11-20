#include <algorithm>
#include <vector>
#include <cassert>
#include <cstdio>
struct Vdp1ModelSource {unsigned polygonCount=2;bool textures=true;bool texturesValid()const{return textures;}} model;
enum class ResidentVdp1Model {None,LiveTown};
ResidentVdp1Model g_residentVdp1Model=ResidentVdp1Model::LiveTown;
int token;
void* g_vdp1SubdivVertices=&token;
void* g_vdp1SubdivIndices=&token;
void* g_gouraudSubdivVertexProgram=&token;
void* g_texturedGouraudSubdivFragmentProgram=&token;
void* g_vdp1GouraudVertices=&token;
void* g_vdp1TextureIndices=&token;
unsigned g_vdp1SubdivVertexCapacity=18,g_vdp1SubdivIndexCapacity=48;
bool fullSubdivMode=true,g_vdp1TexturedReady=true,g_staticRoomCpuReady=false;
struct {bool lightingValid=false;} g_staticRoomCpuMesh;
struct LivePolygonLightState {bool valid=false;};
std::vector<LivePolygonLightState> g_liveTownPolygonLights{{true}};
Vdp1ModelSource liveTownVdp1Source(){return model;}
#include "subdivision_ready.inc"
bool route(){
#include "subdivision_route.inc"
    return subdivisionOnly;
}
int main(){
    assert(route());
    for(auto* pointer:{&g_vdp1SubdivVertices,&g_vdp1SubdivIndices,
        &g_gouraudSubdivVertexProgram,&g_texturedGouraudSubdivFragmentProgram,
        &g_vdp1GouraudVertices,&g_vdp1TextureIndices}) {
        *pointer=nullptr;assert(!route());*pointer=&token;
    }
    for(auto* value:{&fullSubdivMode,&g_vdp1TexturedReady,&model.textures}) {
        *value=false;assert(!route());*value=true;
    }
    g_residentVdp1Model=ResidentVdp1Model::None;assert(!route());
    g_residentVdp1Model=ResidentVdp1Model::LiveTown;
    --g_vdp1SubdivVertexCapacity;assert(!route());++g_vdp1SubdivVertexCapacity;
    --g_vdp1SubdivIndexCapacity;assert(!route());++g_vdp1SubdivIndexCapacity;
    g_liveTownPolygonLights[0].valid=false;assert(!route());
    g_staticRoomCpuMesh.lightingValid=true;assert(!route());
    g_staticRoomCpuReady=true;assert(route());
    std::puts("PASS production subdivision stream gate: shaders, buffers, capacities, mode, textures, lighting and fallback");
}

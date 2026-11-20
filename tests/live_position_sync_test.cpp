#include <vector>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
struct Vertex{float x,y,z;unsigned color;};
struct Texture{float x,y,z,u,v;};
struct Gouraud{float x,y,z;unsigned payload;};
struct Mesh{std::vector<Vertex> vertices;}g_liveTownCpuMesh;
bool g_liveTownStaticRebuilt=false;
bool subdivisionOnly=false;
std::size_t g_liveTownStaticVertexCount=2;
Vertex g_vdp1Vertices[6];Texture g_vdp1TextureVertices[6];Gouraud g_vdp1GouraudVertices[6];
// No legacy-lighting-buffer declaration: this fixture must not need it.
unsigned g_profilePositionSyncVertices=0,g_profilePositionSyncUs=0;
unsigned g_profilePositionSyncLegacyVertices=0;
uint64_t sceKernelGetProcessTimeWide(){static uint64_t t=0;return ++t;}
void sync(){
#include "live_position_sync.inc"
}
int main(){
    for(unsigned i=0;i<6;++i){
        g_liveTownCpuMesh.vertices.push_back({float(i+1),float(i+2),float(i+3),i+100});
        g_vdp1Vertices[i]={-1,-1,-1,0};
        g_vdp1TextureVertices[i]={-1,-1,-1,123,456};
        g_vdp1GouraudVertices[i]={-1,-1,-1,789};
    }
    sync();assert(g_profilePositionSyncVertices==4&&g_profilePositionSyncUs==1);
    assert(g_profilePositionSyncLegacyVertices==4);
    for(unsigned i=0;i<6;++i){
        const auto& src=g_liveTownCpuMesh.vertices[i];
        assert(g_vdp1Vertices[i].x==(i<2?-1:src.x));
        assert(g_vdp1Vertices[i].y==(i<2?-1:src.y));
        assert(g_vdp1Vertices[i].z==(i<2?-1:src.z));
        assert(g_vdp1Vertices[i].color==(i<2?0:src.color));
        assert(g_vdp1TextureVertices[i].x==(i<2?-1:src.x));
        assert(g_vdp1TextureVertices[i].y==(i<2?-1:src.y));
        assert(g_vdp1TextureVertices[i].z==(i<2?-1:src.z));
        assert(g_vdp1GouraudVertices[i].x==(i<2?-1:src.x));
        assert(g_vdp1GouraudVertices[i].y==(i<2?-1:src.y));
        assert(g_vdp1GouraudVertices[i].z==(i<2?-1:src.z));
        assert(g_vdp1TextureVertices[i].u==123&&g_vdp1TextureVertices[i].v==456);
        assert(g_vdp1GouraudVertices[i].payload==789);
    }
    g_liveTownCpuMesh.vertices[2]={200,300,400,500};sync();
    assert(g_vdp1Vertices[2].color==500&&g_vdp1TextureVertices[2].y==300&&g_vdp1GouraudVertices[2].z==400);
    assert(g_vdp1Vertices[0].x==-1); // still excludes retained static prefix
    g_liveTownStaticRebuilt=true;sync();assert(g_profilePositionSyncVertices==6);
    for(unsigned i=0;i<6;++i)assert(g_vdp1Vertices[i].x==g_liveTownCpuMesh.vertices[i].x);
    subdivisionOnly=true;g_liveTownStaticRebuilt=false;
    const auto oldTexture=g_vdp1TextureVertices[2];
    const auto oldGouraud=g_vdp1GouraudVertices[2];
    g_liveTownCpuMesh.vertices[2]={999,888,777,666};sync();
    assert(g_profilePositionSyncLegacyVertices==0);
    assert(g_vdp1Vertices[2].x==999 && g_vdp1Vertices[2].color==666);
    assert(!std::memcmp(&oldTexture,&g_vdp1TextureVertices[2],sizeof(Texture)));
    assert(!std::memcmp(&oldGouraud,&g_vdp1GouraudVertices[2],sizeof(Gouraud)));
    // A mode/shader/capacity/lighting fallback immediately catches up with the
    // current CPU positions, not the last frame written to the legacy stream.
    subdivisionOnly=false;sync();
    assert(g_profilePositionSyncLegacyVertices==4);
    assert(g_vdp1TextureVertices[2].x==999 && g_vdp1GouraudVertices[2].z==777);
    g_liveTownCpuMesh.vertices.clear();g_liveTownStaticRebuilt=false;
    sync();assert(g_profilePositionSyncVertices==0); // no unsigned underflow
    std::puts("Actual live position sync preserves active buffers, static prefix, attributes and rebuilds PASS");
}

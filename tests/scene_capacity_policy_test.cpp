#include "lagi/native_scene_mode.h"
#include "lagi/debug_mesh.h"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdio>
struct Policy {std::size_t polygons,vertices,indices;bool reserved;};
Policy policy(std::size_t polygons,int mode,bool preserve,bool supported=true) {
    struct {std::size_t polygonCount;} model{polygons};
    const unsigned vertexCount=unsigned(polygons*6);
    const auto genericIndexCount=polygons*8;
    const bool subdivBuffersRequired=supported;
    const bool preserveResidentTextures=preserve;
    const int g_sceneGameMode=mode;
#include "capacity_policy.inc"
    return {allocationPolygons,allocationVertices,allocationIndices,reserveSceneCapacity};
}
int main() {
    constexpr std::size_t limit=65535/24;
    for(int mode=1;mode<=4;++mode)for(std::size_t n=1;n<=limit;++n) {
        const auto p=policy(n,mode,true);
        assert(p.reserved && p.polygons==limit && p.vertices==limit*6 && p.indices==limit*8);
        assert(p.polygons>=n && p.polygons*24<=65535 && p.polygons*9<=65535);
    }
    // Flight -> multi-actor battle -> field: no capacity-growth reallocation.
    const auto flight=policy(1191,3,true),battle=policy(2222,3,true),exit=policy(1341,3,true);
    assert(flight.vertices==battle.vertices && flight.indices==exit.indices);
    for(int mode:{0,5,6,7}) {
        const auto p=policy(1191,mode,true);assert(!p.reserved && p.polygons==1191);
    }
    assert(policy(1191,3,false).polygons==1191);
    assert(policy(2731,3,true,false).polygons==2731); // unsupported subdivision: no clamp/drop
    // Actual production base payload layout bounds the added reservation.
    const auto bytesPerQuad=12*sizeof(lagi::azel::DebugColorVertex)+8*sizeof(unsigned short)+
        6*sizeof(lagi::azel::DebugTextureVertex)+6*sizeof(lagi::azel::DebugGouraudPayloadVertex)+
        6*sizeof(unsigned short)+4*sizeof(lagi::azel::DebugColorVertex)+4*sizeof(unsigned short)+
        9*8*sizeof(float)+24*sizeof(unsigned short);
    std::printf("PASS production scene capacity: all native modes, 1..%zu quads, exact active/fallback preservation; added payload reservation=%zu bytes before page rounding\n",
        limit,(limit-2048)*bytesPerQuad);
}

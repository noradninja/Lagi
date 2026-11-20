#include "../src/integration/lagi_live_model_adapter.cpp"
#include <cstdio>
int main() {
    LagiProcessed3dModelPrefix input{};
    input.vertices={{{1,2,3}},{{4,5,6}},{{7,8,9}},{{10,11,12}}};
    input.numVertices=4;
    input.quads.resize(2);
    input.quads[0].indices={0,1,2,3};
    input.quads[1].indices={3,2,1,0};
    lagi::azel_bridge::LiveVdp1Model output;
    assert(lagi::azel_bridge::adapt_processed_model(
        reinterpret_cast<sProcessed3dModel*>(&input),output));
    const unsigned expected[]={0,1,2,0,2,5,5,2,1,5,1,0};
    assert(output.positionSources.size()==12);
    for(unsigned i=0;i<12;++i) {
        assert(output.positionSources[i]==expected[i]);
        const auto& a=output.vertices[i];
        const auto& b=output.vertices[expected[i]];
        assert(a.x==b.x && a.y==b.y && a.z==b.z);
    }
    assert(output.vertices[0].r!=output.vertices[11].r);
    input.quads[1].indices[2]=4;
    assert(!lagi::azel_bridge::adapt_processed_model(
        reinterpret_cast<sProcessed3dModel*>(&input),output));
    std::puts("PASS production native position identity, reversed faces, per-face colors and invalid indices");
}

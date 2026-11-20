#include "lagi/lagi_render_bridge.h"
#include "lagi/lagi_live_model_adapter.h"
#include "lagi/model_snapshot_cache.h"
#include <unordered_map>
#include <cassert>
#include <cstdio>
struct sProcessed3dModel {};
namespace lagi::azel_bridge {
using NativeModelCache=lagi::ModelSnapshotCache<sProcessed3dModel*,LiveVdp1Model>;
static NativeModelCache g_modelCache;
static std::unordered_map<std::int8_t,std::uint64_t> g_bundleGenerations;
static std::unordered_map<std::int8_t,std::shared_ptr<const std::vector<std::uint8_t>>> g_bundleTextureMemory;
static std::unordered_map<std::int8_t,std::uint32_t> g_bundleTextureEpochs;
static std::unordered_map<std::uint64_t,RegisteredModelResource> g_registeredResources;
static bool g_resourceInventoryDirty=false;
static unsigned adaptations=0;
bool adapt_processed_model(sProcessed3dModel*,LiveVdp1Model& out) {
    out.vertices.push_back({float(++adaptations),0,0,1,2,3,255});return true;
}
#include "registration.inc"
}
int main() {
    using namespace lagi::azel_bridge;
    sProcessed3dModel model;
    g_bundleGenerations[2]=1;g_bundleTextureEpochs[2]=7;
    g_bundleTextureMemory[2]=std::make_shared<const std::vector<std::uint8_t>>(16,3);
    register_native_model_resource(2,0x100,&model);
    const auto key=(std::uint64_t(2)<<32)|0x100;
    auto published=g_registeredResources.at(key);
    auto first=g_modelCache.find(&model);
    assert(adaptations==1 && published.model.get()==&first->value);
    assert(published.textureWriteEpoch==7 && g_resourceInventoryDirty);
    g_resourceInventoryDirty=false;
    register_native_model_resource(2,0x100,&model);
    assert(adaptations==1 && !g_resourceInventoryDirty);
    g_modelCache.retire(&model);first.reset();
    g_registeredResources.clear();g_bundleGenerations[2]=2;
    register_native_model_resource(2,0x100,&model);
    auto second=g_modelCache.find(&model);
    assert(adaptations==2 && published.model->vertices[0].x==1);
    assert(second->value.vertices[0].x==2);
    assert(g_registeredResources.at(key).model.get()==&second->value);
    assert(g_registeredResources.at(key).bundleGeneration==2);
    register_native_model_resource(3,0x100,&model);
    assert(adaptations==2);
    std::puts("PASS production registration shares first-draw snapshot, duplicate registration, retirement and address reuse");
}

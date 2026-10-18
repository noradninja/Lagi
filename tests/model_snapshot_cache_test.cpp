#include "lagi/model_snapshot_cache.h"
#include <cassert>
#include <cstdio>
#include <vector>

int main() {
    lagi::ModelSnapshotCache<int*, std::vector<int>> cache;
    int native;
    auto first = cache.insert(&native, {1, 2, 3});
    assert(cache.find(&native) == first);
    assert(cache.insert(&native, {99}) == first);
    std::weak_ptr<const decltype(cache)::Snapshot> lifetime = first;
    std::vector<decltype(first)> published{first};
    cache.retire(&native);
    assert(!cache.find(&native));
    // Identical native address and polygon count must still be a new resource.
    auto second = cache.insert(&native, {4, 5, 6});
    assert(second->generation > first->generation);
    assert(published[0]->value[0] == 1 && second->value[0] == 4);
    first.reset();
    assert(!lifetime.expired());
    published.clear();
    assert(lifetime.expired());
    cache.retire(nullptr);
    cache.retire(&native);
    assert(second->value[2] == 6);
    std::puts("PASS model retirement, address reuse, and published-frame lifetime");
}

#include "neptune/geometry_resource_key.h"

#include <cassert>
#include <cstdio>
#include <unordered_map>

int main() {
    using namespace neptune;
    GeometryRepresentationKey key{};
    assert(!key.valid());
    key.source = {1, 2, 3};
    key.layout = 1;
    key.coordinateConvention = 1;
    assert(key.valid());

    GeometryRepresentationKeyHash hash;
    const auto same = key;
    assert(same == key && hash(same) == hash(key));

    std::unordered_map<GeometryRepresentationKey, int,
        GeometryRepresentationKeyHash> entries;
    entries.emplace(key, 42);
    assert(entries.at(same) == 42);
    auto distinct = [&](GeometryRepresentationKey other) {
        assert(other.valid());
        assert(!(other == key));
        assert(entries.find(other) == entries.end());
    };
    auto other = key;
    ++other.source.producer; distinct(other);
    other = key; ++other.source.resource; distinct(other);
    other = key; ++other.source.revision; distinct(other);
    other = key; ++other.layout; distinct(other);
    other = key; ++other.coordinateConvention; distinct(other);
    other = key; ++other.bakedAttributes; distinct(other);

    other = key; other.transform[3] = 65536;
    assert(!other.valid()); // Local-space keys must be canonical.
    other.space = GeometrySpace::Transformed; distinct(other);
    entries.emplace(other, 7);
    auto moved = other;
    moved.transform[3] = -65536;
    assert(moved.valid() && !(moved == other));
    assert(entries.find(moved) == entries.end());
    assert(entries.at(other) == 7);
    other = key; other.space = static_cast<GeometrySpace>(255);
    assert(!other.valid());
    other = key; other.source.producer = 0; assert(!other.valid());
    other = key; other.source.resource = 0; assert(!other.valid());
    other = key; other.source.revision = 0; assert(!other.valid());
    other = key; other.layout = 0; assert(!other.valid());
    other = key; other.coordinateConvention = 0; assert(!other.valid());
    std::puts("PASS Neptune geometry resource and representation keys");
}

#include "neptune/geometry_resource_registry.h"
#include <cassert>
#include <cstdio>

int main() {
    using namespace neptune;
    GeometryResourceRegistry registry(5), foreign(6), invalid(0);
    assert(!invalid.create(1).valid());
    assert(!registry.create(0).valid());
    auto a = registry.create(10), b = registry.create(10);
    auto c = registry.create(20), f = foreign.create(10);
    assert(a.valid() && b.valid() && c.valid());
    assert(a.resource != b.resource && a.resource != c.resource);
    assert(!registry.current(f));
    assert(!registry.retire(f));
    assert(!registry.revise(f).valid());
    auto updated = registry.revise(a);
    assert(updated.valid() && updated.resource == a.resource);
    assert(updated.revision == a.revision + 1);
    assert(!registry.current(a) && registry.current(updated));
    assert(!registry.revise(a).valid() && !registry.retire(a));
    assert(registry.retire(b) && !registry.retire(b));
    assert(registry.retireOwner(10) == 1);
    assert(!registry.current(updated) && registry.current(c));
    assert(registry.retireOwner(10) == 0);
    auto replacement = registry.create(10);
    assert(replacement.resource != a.resource && replacement.resource != b.resource);
    assert(registry.current(replacement) && registry.size() == 2);
    assert(registry.retireOwner(0) == 0);
    assert(registry.retireOwner(20) == 1);
    assert(registry.retire(replacement) && registry.size() == 0);
    std::puts("PASS Neptune resource registry lifecycle");
}

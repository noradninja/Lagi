#include "neptune/geometry_submission_epoch.h"
#include "neptune/geometry_arena.h"
#include <cassert>
#include <cstdio>

int main() {
    neptune::GeometryArena<8> arena(1024);
    neptune::GeometrySubmissionEpoch epoch;
    auto serial=epoch.begin();
    assert(serial==1 && epoch.active()==serial);
    auto range=arena.allocate(1024,64);
    assert(range && arena.markUsed(range,serial) && arena.retire(range));
    assert(!arena.allocate(1,1));
    // An intermediate wait is intentionally not reported as completion.
    assert(epoch.active()==serial && arena.used()==1024);
    auto done=epoch.complete(true);
    assert(done==serial && arena.completeThrough(done));
    assert(arena.used()==0 && epoch.begin()==2);
    range=arena.allocate(1024,64);
    assert(arena.markUsed(range,epoch.active()) && arena.retire(range));
    assert(!epoch.complete(false) && epoch.failed() && !epoch.active());
    assert(!epoch.begin() && !epoch.complete(true));
    assert(arena.used()==1024 && !arena.allocate(1,1));
    neptune::GeometrySubmissionEpoch nested;
    assert(nested.begin()==1 && !nested.begin() && nested.failed());
    std::puts("PASS Neptune geometry submission epoch");
}

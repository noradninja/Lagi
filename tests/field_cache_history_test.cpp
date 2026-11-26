#include "lagi/field_cache_history.h"
#include <cassert>
#include <iostream>
using namespace lagi;
static FieldCacheVariant v(unsigned id,unsigned offset=0x100) {
    FieldCacheVariant a;a.placement={-1,2,id};a.offset=offset;a.model=offset;a.generation=1;a.polygons=12;a.hasMatrix=true;return a;
}
int main() {
    FieldCacheHistory h;
    h.begin();h.add(v(1));h.add(v(2));h.add(v(3));auto d=h.finish();
    assert(h.initialized()&&!d.added&&!d.lod);
    h.begin();h.add(v(1));h.add(v(2,0x104));h.add(v(3));d=h.finish();
    assert(d.lod==1&&!d.added&&!d.removed&&!d.resource&&!d.reordered);
    h.begin();h.add(v(3));h.add(v(4));h.add(v(2,0x104));d=h.finish();
    assert(d.added==1&&d.removed==1&&d.reordered&&!d.lod);
    auto changed=v(2,0x104);changed.generation++;changed.matrix[3]=123;
    h.begin();h.add(v(3));h.add(v(4));h.add(changed);d=h.finish();
    assert(d.resource==1&&d.transform==1&&!d.lod&&!d.reordered);
    h.begin();h.add(v(3));h.add(v(3));d=h.finish();assert(d.ambiguous);
    h.begin();h.add(v(3));d=h.finish();assert(d.ambiguous);
    h.reset();assert(!h.initialized());
    h.begin();h.add(v(1));h.finish();
    h.begin();h.add(v(4));h.add(v(1));d=h.finish();assert(d.added==1&&!d.reordered);
    h.begin();h.add(v(1));d=h.finish();assert(d.removed==1&&!d.reordered);
    std::cout<<"PASS field placement delta classification\n";
}

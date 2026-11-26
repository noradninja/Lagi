#include "lagi/field_placement_cache.h"
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
struct Vertex {float x,y,z;std::uint8_t r,g,b,a;};
using namespace lagi;
static FieldPlacementKey key(unsigned object,unsigned offset=0xa44) {
    FieldPlacementKey k;k.model=offset;k.generation=1;k.cell=4;k.object=object;
    k.offset=offset;k.vertices=2;k.polygons=1;k.matrix[0]=65536;return k;
}
int main(){
    FieldPlacementCache<Vertex> c(6*sizeof(Vertex),3);
    Vertex nearV[]={{1,2,3,9,8,7,255},{1,2,3,4,5,6,255}};
    Vertex farV[]={{4,5,6,1,2,3,255},{7,8,9,4,5,6,255}};
    auto a=key(1),b=key(1,0xa58),d=key(2),e=key(3);
    assert(!c.find(a));assert(c.store(a,nearV,2));assert(c.store(b,farV,2));
    assert(!std::memcmp(c.find(a),nearV,sizeof nearV));
    assert(!std::memcmp(c.find(b),farV,sizeof farV));
    auto changed=a;changed.generation++;assert(!c.find(changed));
    changed=a;changed.matrix[3]++;assert(!c.find(changed));
    changed=a;changed.polygons++;assert(!c.find(changed));
    changed=a;changed.vertices++;assert(!c.find(changed));assert(!c.store(changed,nearV,2));
    changed=a;changed.bundle=2;assert(!c.find(changed));
    changed=a;changed.cell++;assert(!c.find(changed));
    changed=a;changed.generation=0;assert(!c.store(changed,nearV,2));
    assert(c.store(d,nearV,2));assert(c.find(a));assert(c.find(b));
    assert(c.store(e,nearV,2));assert(!c.find(d));assert(c.evictions()==1);
    assert(c.bytes()==6*sizeof(Vertex)&&c.size()==3);
    FieldPlacementCache<Vertex> small(sizeof(Vertex),10);
    assert(!small.store(a,nearV,2));assert(!small.size());
    FieldPlacementCache<Vertex> limited(100*sizeof(Vertex),1);
    assert(limited.store(a,nearV,2));assert(limited.store(b,farV,2));
    assert(!limited.find(a)&&limited.find(b)&&limited.evictions()==1);
    // Cache owns the copies; frame scratch data may be overwritten.
    nearV[0].x=999;assert(c.find(a)[0].x==1);
    c.clear();assert(!c.size()&&!c.bytes()&&!c.evictions()&&!c.find(a));
    std::cout<<"PASS placement variants, exact keys, owned payload, LRU, byte and entry limits\n";
}

#include "lagi/native_formation_motion.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <limits>

// Independent scalar rendering of the original SH2 load/sub/mask/multiply/
// add/store sequence. Signed division explicitly rounds down, unlike C++ /.
static int32_t bits(uint32_t v) {
    if(v<=0x7fffffffu)return int32_t(v);
    return int32_t(int64_t(v)-0x100000000ll);
}
static int32_t mul(int32_t a,int32_t b) {
    int64_t p=int64_t(a)*b;
    int64_t q=p/65536;
    if(p<0 && p%65536)--q;
    return bits(uint32_t(q));
}
static lagi::NativeFormationSpringAxis reference(lagi::NativeFormationSpringAxis s,
                                                int32_t center,int32_t spring,int32_t damping) {
    uint32_t r8=uint32_t(s.target)-uint32_t(s.current);
    uint32_t r4=r8;
    if(r4&0x08000000u)r4|=0xf0000000u;else r4&=0x0fffffffu;
    uint32_t r0=uint32_t(mul(bits(r4),spring));
    uint32_t r2=uint32_t(s.acceleration)+r0;
    s.acceleration=bits(r2);
    r0=uint32_t(mul(s.velocity,damping));
    s.acceleration=bits(uint32_t(s.acceleration)-r0);
    s.velocity=bits(uint32_t(s.velocity)+uint32_t(s.acceleration));
    s.current=bits(uint32_t(s.current)+uint32_t(s.velocity));
    s.world=bits(uint32_t(center)+uint32_t(s.current));
    s.acceleration=0;
    return s;
}
static void check(lagi::NativeFormationSpringAxis initial,int32_t center,int32_t spring,int32_t damping) {
    auto expected=reference(initial,center,spring,damping);
    lagi::stepNativeFormationSpring(initial,center,spring,damping);
    assert(initial.target==expected.target&&initial.current==expected.current);
    assert(initial.velocity==expected.velocity&&initial.acceleration==expected.acceleration);
    assert(initial.world==expected.world);
}
int main() {
    const std::array<int32_t,15> edge={0,1,-1,65535,65536,-65536,
        0x07ffffff,0x08000000,0x0fffffff,0x10000000,
        int32_t(-0x08000000),int32_t(-0x08000001),
        std::numeric_limits<int32_t>::min(),std::numeric_limits<int32_t>::max(),0x12345678};
    size_t count=0;
    for(auto a:edge)for(auto b:edge)for(auto v:edge)for(auto k:edge)for(auto d:edge){
        check({a,b,v,b,a},v,k,d);++count;
    }
    uint32_t seed=0x73942u;
    auto random=[&](){seed=seed*1664525u+1013904223u;return bits(seed);};
    for(int i=0;i<100000;++i){
        lagi::NativeFormationSpringAxis s{random(),random(),random(),random(),random()};
        auto center=random(),spring=random(),damping=random();check(s,center,spring,damping);++count;
    }
    for(int route=0;route<64;++route){
        lagi::NativeFormationSpringAxis s{random(),random(),random(),random(),random()},r=s;
        for(int frame=0;frame<600;++frame){
            if(frame%37==0)s.target=r.target=random();
            auto center=random();r=reference(r,center,0xff,0xfff);
            lagi::stepNativeFormationSpring(s,center,0xff,0xfff);
            assert(s.current==r.current&&s.velocity==r.velocity&&s.world==r.world);++count;
        }
    }
    std::printf("PASS: %zu native formation spring boundary/random/trajectory cases; scalar SH2 recurrence, not device execution\n",count);
}

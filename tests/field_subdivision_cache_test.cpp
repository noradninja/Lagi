#include "lagi/field_subdivision_cache.h"
#include <cassert>
#include <cstdio>
#include <cstring>

static void fill(float a,float b,float c,float d,float out[9]) {
    out[0]=a;out[1]=(a+b)*0.5f;out[2]=b;
    out[3]=(a+d)*0.5f;out[5]=(b+c)*0.5f;
    out[6]=d;out[7]=(d+c)*0.5f;out[8]=c;
    out[4]=(out[1]+out[7])*0.5f;
}
int main() {
    lagi::FieldSubdivPositionSlot slot;
    std::array<float,12> a={1,2,3,4,5,6,7,8,9,10,11,12};
    assert(!slot.matches(a));slot.remember(a);assert(slot.matches(a));
    auto b=a;b[8]+=1;assert(!slot.matches(b));assert(slot.matches(a));
    slot.invalidate();assert(!slot.matches(a));slot.remember(b);assert(slot.matches(b));
    a[0]=0.0f;b=a;b[0]=-0.0f;slot.remember(a);assert(!slot.matches(b));
    lagi::FieldSubdivUvCache cache;
    for(unsigned w:{8u,24u,48u,504u})for(unsigned h:{1u,24u,48u,255u})
        for(unsigned flip=0;flip<4;++flip) {
            float u0=0.5f/float(w),v0=0.5f/float(h);
            float u1=(float(w)-0.5f)/float(w),v1=(float(h)-0.5f)/float(h);
            if(flip&1u){float t=u0;u0=u1;u1=t;}
            if(flip&2u){float t=v0;v0=v1;v1=t;}
            float u[9],v[9];fill(u0,u1,u1,u0,u);fill(v0,v0,v1,v1,v);
            bool hit=false;
            const auto& g=cache.get(w,h,flip,fill,hit);
            assert(!std::memcmp(u,g.u,sizeof u)&&!std::memcmp(v,g.v,sizeof v));
            const auto& again=cache.get(w,h,flip,fill,hit);
            assert(hit&&!std::memcmp(u,again.u,sizeof u)&&!std::memcmp(v,again.v,sizeof v));
        }
    std::puts("PASS exact subdivision slot identity and UV templates");
}

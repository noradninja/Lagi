#include "../include/lagi/motion_cadence.h"
#include <cassert>
#include <climits>
#include <cstdio>
int main(){
    lagi::MotionCadence<1,1> w;
    assert(!w.add(1,1,100,{0},{INT_MIN}));
    assert(!w.add(1,2,110,{1},{INT_MAX}));
    assert(w.summary.maxBStep==4294967295ull);
    assert(!w.add(1,4,150,{2},{INT_MAX}));
    assert(w.summary.serialGaps==1 && w.summary.aChangedBRepeated==1);
    assert(w.summary.minUs==10 && w.summary.maxUs==40);
    assert(w.summary.minBStep==0);
    w.nextWindow();
    assert(!w.add(1,5,180,{2},{INT_MAX}));
    assert(w.summary.intervals==1 && w.summary.aRepeated==1 && w.summary.bRepeated==1);
    assert(w.summary.minUs==30); // endpoint preserved through logging boundary
    assert(!w.add(2,6,200,{0},{0}));
    assert(w.summary.intervals==0); // scene/water boundary has no interval
    assert(!w.add(2,7,190,{0},{0}));
    assert(w.summary.intervals==0); // clock regression starts a baseline
    w.reset();
    assert(!w.add(2,10,1000,{0},{0}));
    for(unsigned i=1;i<=120;++i)
        assert(w.add(2,10+i,1000+33367ull*i,{int(i)},{int(i)})==(i==120));
    assert(w.summary.totalUs==33367ull*120 && w.summary.serialGaps==0);
    std::puts("PASS exact snapshot cadence, repeats, sequence gaps, signed steps, boundaries and 120-interval windows");
}

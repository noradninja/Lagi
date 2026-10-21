#include "../include/lagi/play_time_clock.h"
#include <cassert>
#include <cstdio>
struct s_moduleManager{};
struct {struct {std::uint32_t m3C_frameCounter=0;} gameStats;} mainGameState;
struct {std::uint32_t mC_numFramesPresented=0;}vblankData;
#include "play_time_native_consumer.inc"
static std::uint32_t run(unsigned iterations,unsigned interval){
    lagi::PlayTimeClock clock;clock.sample(1000);mainGameState.gameStats.m3C_frameCounter=0;
    for(unsigned i=1;i<=iterations;++i){
        vblankData.mC_numFramesPresented=clock.sample(1000+std::uint64_t(i)*interval);
        moduleManager_Update(nullptr);vblankData.mC_numFramesPresented=0;
    }
    return mainGameState.gameStats.m3C_frameCounter;
}
int main(){
    assert(run(18000,16667)==18000); // five minutes at 60-ish frames/sec
    assert(run(9000,33334)==18000); // same elapsed play time at 30-ish
    assert(run(6000,50000)==18000); // same at 20 FPS
    const auto ticks=run(6000,50000);assert(ticks/3600==5);
    lagi::PlayTimeClock c;assert(c.sample(100)==0);
    assert(c.sample(1000100)==60);assert(c.sample(500)==0);assert(c.sample(1000500)==60);
    c={};c.sample(0);std::uint64_t total=0;
    for(unsigned i=1;i<=1000000;++i)total+=c.sample(i);
    assert(total==60); // sub-tick deltas retain every fractional remainder
    std::puts("PASS elapsed 60 Hz service and actual native time consumer at 20/30/60 FPS, save-minute units, fractions and reset");
}

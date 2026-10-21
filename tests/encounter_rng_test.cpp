#include <cassert>
#include <cstdint>
#include <cstdio>
#include <initializer_list>
using u32=std::uint32_t;using u16=std::uint16_t;
bool isTraceEnabled(){return false;}
bool delayTrace=false;
void readTraceLogU32(u32,const char*){assert(false);}
u32 randomNumber();
#include "rng_body.inc"
u32 reference(u32& seed){
    const u32 working=(seed&65535u)?seed:0x2a6d365bu;
    const u32 product=working*41u;
    const u32 folded=(product>>16)+(product&65535u);
    seed=(folded<<16)|(product&65535u);
    return (working&0xffff0000u)|(folded&65535u);
}
int main(){
    for(u32 start: {0u,1u,0x10000u,0xffffffffu,0x12345678u}){
        u32 expected=start;reference(expected);initRandomSeed(start);
        assert(RNG_seed==expected);
        for(unsigned i=0;i<200000;++i){
            const u32 want=reference(expected),got=randomNumber();
            assert(got==want && RNG_seed==expected);
            assert(got%100u==want%100u);
        }
    }
    puts("PASS actual generated Azel RNG: one million outputs/state transitions, zero-low-word reseed and unsigned 0..99 modulo; not original-instruction or encounter-frequency proof");
}

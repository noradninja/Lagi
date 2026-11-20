#include "lagi/battle_stick_input.h"
#include <cassert>
#include <cstdio>
int main() {
    using lagi::input_bridge::stick_directions;
    assert(stick_directions(0,0)==0);
    assert(stick_directions(.48f,-.48f)==0);
    assert(stick_directions(-1,0)==0x40);
    assert(stick_directions(1,0)==0x80);
    assert(stick_directions(0,-1)==0x10);
    assert(stick_directions(0,1)==0x20);
    assert(stick_directions(1,-1)==0x90);
    unsigned short previous=0x80;
    unsigned short combined=0x80|stick_directions(1,0);
    assert((combined & ~previous)==0); // D-pad/stick handoff stays held.
    assert((stick_directions(-1,0)&~previous)==0x40);
    std::puts("Shared stick direction checks PASS");
}

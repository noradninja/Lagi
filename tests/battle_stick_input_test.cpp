#include "lagi/battle_stick_input.h"
#include <cassert>
#include <cstdio>
int main() {
    using lagi::input_bridge::battle_stick_directions;
    assert(battle_stick_directions(1,1,false)==0);
    assert(battle_stick_directions(0,0,true)==0);
    assert(battle_stick_directions(.48f,-.48f,true)==0);
    assert(battle_stick_directions(-1,0,true)==0x40);
    assert(battle_stick_directions(1,0,true)==0x80);
    assert(battle_stick_directions(0,-1,true)==0x10);
    assert(battle_stick_directions(0,1,true)==0x20);
    assert(battle_stick_directions(1,-1,true)==0x90);
    unsigned short previous=0x80;
    unsigned short combined=0x80|battle_stick_directions(1,0,true);
    assert((combined & ~previous)==0); // D-pad/stick handoff stays held.
    assert((battle_stick_directions(-1,0,true)&~previous)==0x40);
    std::puts("Battle stick direction checks PASS");
}

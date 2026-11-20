#include "lagi/camp_npc_patrol.h"
#include <cassert>
#include <cstdio>
#include <vector>
using namespace lagi::camp::patrol;
static I divide(I n,I d){assert(d);return I(std::int64_t(n)*65536/d);}
int main()
{
    const Bounds box{0,0,100,100};
    for(U action=0;action<4;++action) {
        State state;Frame f;f.x=f.z=50;f.rate=65536;f.animation=8;
        int randomCalls=0,steps=0;std::vector<int> animations;
        advance(state,f,box,[&](){++randomCalls;return action;},
            [&](I a){animations.push_back(a);},[](I a){assert(a==1);return 38;},divide,
            [&](const Frame& current){assert(current.speed==-113);++steps;});
        assert(randomCalls==1&&state.action==action&&animations.size()==1);
        if(action==0)assert(state.timer==15&&f.animation==0&&f.control==2&&!steps);
        if(action==1)assert(state.timer==90&&f.animation==1&&f.control==1&&steps==1&&f.stepYaw==0);
        if(action==2)assert(state.timer==17&&f.animation==3&&f.control==2&&!steps&&f.yaw==-0x147AE1);
        if(action==3)assert(state.timer==90&&f.animation==1&&f.control==1&&steps==1&&f.yaw==0x147AE1);
    }
    // Boundary turn initializes animation/timer but returns before this frame's
    // yaw, look-at and movement tail. The same RNG word supplies its jitter.
    State turn;Frame f;f.x=-1;f.z=50;f.rate=65536;f.lookYaw=1234;
    int randomCalls=0,plays=0,steps=0;
    advance(turn,f,box,[&](){++randomCalls;return U(0);},[&](I a){assert(a==3);++plays;},
        [](I){assert(false);return 0;},divide,[&](const Frame&){++steps;});
    assert(turn.action==2&&turn.timer==16&&f.stepYaw==-0x147AE1);
    assert(f.yaw==0&&f.lookYaw==1234&&randomCalls==1&&plays==1&&!steps);
    // Angles outside the turn helper's half-open range force walking instead.
    State outside;Frame out;out.x=50;out.z=101;out.rate=65536;
    advance(outside,out,box,[](){return U(0);},[](I a){assert(a==1);},
        [](I){return 38;},divide,[](const Frame&){});
    assert(outside.action==1&&outside.timer==44&&out.control==1&&out.speed==-113);
    for(const auto previous : {2,3}) {
        State state{std::int8_t(previous),0};Frame walk;walk.x=walk.z=50;walk.rate=65536;
        advance(state,walk,box,[](){return U(0);},[](I a){assert(a==1);},
            [](I){return 38;},divide,[](const Frame&){});
        assert(state.action==1&&state.timer==44); // No consecutive randomized turns.
    }
    // Interior/equality boundaries do not restart the native timer. Existing
    // movement consumes no new random choices, and still calls the step service.
    for(I x : {0,50,100})for(I z : {0,50,100})for(U angle=0;angle<4096;angle+=7) {
        State state{1,5};Frame move;move.x=x;move.z=z;move.yaw=I(angle<<16);move.speed=-113;
        int moved=0;
        advance(state,move,box,[](){assert(false);return 0u;},[](I){assert(false);},
            [](I){assert(false);return 0;},divide,[&](const Frame&){++moved;});
        assert(state.timer==4&&moved==1&&move.control==1);
    }
    State boundary{1,5};Frame exiting;exiting.x=-1;exiting.z=50;exiting.speed=-113;
    advance(boundary,exiting,box,[](){assert(false);return 0u;},[](I){assert(false);},
        [](I){assert(false);return 0;},divide,[](const Frame&){});
    assert(boundary.timer==0); // Still steps, then makes a new choice next update.
    assert(add(INT32_MAX,1)==INT32_MIN&&neg(INT32_MIN)==INT32_MIN);
    puts("PASS Camp patrol: all random actions, boundary-turn early return, native integer/fixed durations, repeat-turn prevention, boundary equality and no-RNG movement tail; hardware pending");
}

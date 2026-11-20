#pragma once
#include <bit>
#include <cstdint>

namespace lagi::camp::patrol {
using I=std::int32_t;using U=std::uint32_t;
inline I add(I a,I b){return std::bit_cast<I>(U(a)+U(b));}
inline I neg(I a){return std::bit_cast<I>(0u-U(a));}
inline I mul(I a,I b){return I((std::int64_t(a)*b)>>16);}
inline U heading(I yaw,I offset){return (U(yaw)+U(offset))&0x0FFFFFFFu;}
struct State {std::int8_t action=0;I timer=0;};
struct Frame {
    I x=0,z=0,yaw=0,stepYaw=0,speed=0,lookYaw=0,rate=0;
    I animation=0,control=0;
};
struct Bounds {I minX=0,minZ=0,maxX=0,maxZ=0;};
// CAMP060709E8. Native compiler division takes divisor inR0 and dividend
// inR1; this is an integer turn duration, not a fixed-point interpolation.
template<class Play>
bool begin_boundary_turn(State& state,Frame& f,U relative,U random,Play play)
{
    if(relative>=0x0AAAAAAAu)return false;
    const I jitter=I(random%0x02AAAAAAu);
    if(relative<0x05555555u) {
        f.stepYaw=mul(-0x00147AE1,f.rate);
        state.timer=I((std::int64_t(relative)+jitter)/neg(f.stepYaw));
    } else {
        f.stepYaw=mul(0x00147AE1,f.rate);
        state.timer=I((std::int64_t(0x0AAAAAAA)+jitter-relative)/f.stepYaw);
    }
    if(f.animation!=3){f.animation=3;play(3);}
    f.control=2;state.action=2;
    return true;
}
// CAMP06070ABC. Services preserve Azel's RNG, animation lookup/stepping and
// FP_Div semantics. The caller applies the prior collision solve first.
template<class Random,class Play,class Frames,class Divide,class Step>
void advance(State& state,Frame& f,const Bounds& b,Random random,Play play,
    Frames frames,Divide divide,Step step)
{
    if(state.timer<=0) {
        f.stepYaw=0;f.speed=0;
        const U rng=U(random());
        bool boundary=false;U relative=0;
        if(f.x<b.minX){boundary=true;relative=heading(f.yaw,0x01555555);}
        else if(f.x>b.maxX){boundary=true;relative=heading(f.yaw,I(0xF9555556u));}
        else if(f.z<b.minZ){boundary=true;relative=heading(f.yaw,0x05555555);}
        else if(f.z>b.maxZ){boundary=true;relative=heading(f.yaw,I(0xFD555556u));}
        if(boundary) {
            if(begin_boundary_turn(state,f,relative,rng,play))return;
            state.action=1;
        } else state.action=state.action>=2?1:std::int8_t(rng&3u);
        const I duration=I(rng&15u)+16;
        const I bit=I(rng&1u);
        auto select=[&](I animation){if(f.animation!=animation){f.animation=animation;play(animation);}};
        auto walking_duration=[&](){
            f.speed=mul(-113,f.rate);
            return divide(I(frames(1))*(bit+1),neg(std::bit_cast<I>(U(f.speed)*0x1E1u)));
        };
        switch(state.action) {
        case 0:state.timer=duration;select(0);f.control=2;break;
        case 1:state.timer=walking_duration();select(1);f.control=1;break;
        case 2:
            f.stepYaw=mul(bit?0x00147AE1:-0x00147AE1,f.rate);
            state.timer=duration;select(3);f.control=2;break;
        case 3:
            f.stepYaw=mul(bit?0x00147AE1:-0x00147AE1,f.rate);
            state.timer=walking_duration();select(1);f.control=1;break;
        }
    }
    --state.timer;
    f.yaw=add(f.yaw,f.stepYaw);
    const I desired=f.stepYaw>0?0x01C71C71:f.stepYaw<0?I(0xFE38E38Fu):0;
    f.lookYaw=add(f.lookYaw,mul(add(desired,neg(f.lookYaw)),0x1999));
    if(f.speed!=0) {
        if((f.x<b.minX && heading(f.yaw,0x01555555)<0x0AAAAAAAu) ||
           (f.x>b.maxX && heading(f.yaw,I(0xFEAAAAABu))>0x05555555u) ||
           (f.z<b.minZ && heading(f.yaw,0x05555555)<0x0AAAAAAAu) ||
           (f.z>b.maxZ && heading(f.yaw,0x02AAAAAA)>0x05555555u))
            state.timer=0;
        step(f);f.control=1;
    }
}
}

#pragma once
#include "lagi/native_formation_motion.h"
namespace lagi {
// Native A5 single 0605B558 / dual 0605F922: ordered pairs mutate the
// OTHER position immediately, including self-jitter for actors 0 and 1.
template<class Positions,class Live,class Random,class Distance>
void stepNativeBattleSpacing(Positions& positions,int count,Live live,Random random,Distance distance) {
    for(int i=0;i<count;++i) {
        if(!live(i))continue;
        for(int j=0;j<count;++j) {
            if(!live(j))continue;
            auto& other=positions[j];
            auto add=[&](unsigned axis,std::int32_t value) {
                other[axis]=formationAdd(other[axis].asS32(),value);
            };
            if(i==j) {
                if(j<2){add(0,std::int32_t(random()&0x1FFF)-0x1000);add(2,std::int32_t(random()&0x1FFF)-0x1000);}
            } else {
                const auto dx=formationSubtract(positions[i][0].asS32(),other[0].asS32());
                const auto dz=formationSubtract(positions[i][2].asS32(),other[2].asS32());
                const auto separation=distance(positions[i],other);
                if(separation<0xA000){add(0,dx<0?0x1000:-0x1000);add(2,dz<0?0x1000:-0x1000);}
                else if(separation<0x32000){add(0,std::int32_t(random()&0x1FFF)-0x1000);add(2,std::int32_t(random()&0x1FFF)-0x1000);}
                else{add(0,dx<0?-0x2000:0x2000);add(2,dz<0?-0x4000:0x4000);}
            }
            if(other[0].asS32()>=0x19000)other[0]=0x19000;
            if(other[0].asS32()<=-0x19000)other[0]=-0x19000;
            if(other[2].asS32()>=0x14000)other[2]=0x14000;
            if(other[2].asS32()<=-0x14000)other[2]=-0x14000;
        }
    }
}
}

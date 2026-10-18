#pragma once
// Native BTL_A3 projectile descriptor (+00..+2C), usable for disc-backed
// descriptors and the two original stack-built urchin attack patterns.
struct sLagiBattleProjectileDescriptor
{
    u8 slot{};
    u16 hierarchy{}, pose{}, animation{};
    sSaturnPtr quad{};
    s16 travel{}, lifetime{};
    fixedPoint horizontalAcceleration{}, verticalAcceleration{};
    s16 damage{};
    u8 damageType{};
    sSaturnPtr muzzle{}, trail{}, hit{};
    s16 trailMask{};
    u8 muzzleSlot{}, trailSlot{}, hitSlot{};
};

static sLagiBattleProjectileDescriptor lagiReadBattleProjectileDescriptor(sSaturnPtr ptr)
{
    sLagiBattleProjectileDescriptor d;
    d.slot=readSaturnU8(ptr);
    d.hierarchy=readSaturnU16(ptr+2);
    d.pose=readSaturnU16(ptr+4);
    d.animation=readSaturnU16(ptr+6);
    d.quad=readSaturnEA(ptr+8);
    d.travel=readSaturnS16(ptr+0xC);
    d.lifetime=readSaturnS16(ptr+0xE);
    d.horizontalAcceleration=readSaturnFP(ptr+0x10);
    d.verticalAcceleration=readSaturnFP(ptr+0x14);
    d.damage=readSaturnS16(ptr+0x18);
    d.damageType=readSaturnU8(ptr+0x1A);
    d.muzzle=readSaturnEA(ptr+0x1C);
    d.trail=readSaturnEA(ptr+0x20);
    d.hit=readSaturnEA(ptr+0x24);
    d.trailMask=readSaturnS16(ptr+0x28);
    d.muzzleSlot=readSaturnU8(ptr+0x2A);
    d.trailSlot=readSaturnU8(ptr+0x2B);
    d.hitSlot=readSaturnU8(ptr+0x2C);
    return d;
}

// 0605665E / 060567EE. Emit synchronously consumes each descriptor and copies
// both positions, just as the native constructor does before its caller loops.
// Keep this independent of task ownership so its exact RNG/service order can
// be checked before connecting the recovered projectile lifecycle.
template<class Emit>
static void lagiBuildUrchinProjectilePattern(const sVec3_FP& source,
    const sVec3_FP& dragon, bool alternate, bool strong, Emit emit)
{
    sLagiBattleProjectileDescriptor d;
    d.slot=12;d.hierarchy=12;d.pose=0xDC;
    d.damage=alternate?9:20;
    d.lifetime=alternate?60:30;
    d.verticalAcceleration=-0x160;
    d.hit=g_BTL_A3->getSaturnPtr(alternate?0x060A8680:0x060AE668);
    if(alternate){
        d.trail=g_BTL_A3->getSaturnPtr(0x060A87B4);
        d.trailMask=1;d.trailSlot=12;d.hitSlot=12;
    }
    const int count=alternate?(strong?16:8):(strong?20:8);
    int exactTargets=strong?3:2;
    for(int i=0;i<count;++i){
        auto target=dragon;
        if(alternate){
            for(int axis=0;axis<3;++axis)
                target[axis]+=fixedPoint(s32(randomNumber()&0x7FFF)-0x3FFF);
            d.horizontalAcceleration=fixedPoint(s32(randomNumber()&0x3FF)-0x1FF);
            d.verticalAcceleration=fixedPoint(s32(randomNumber()&0x3FF)-0x1FF);
            d.travel=s16((randomNumber()&0x1F)+30);
        }else{
            if(--exactTargets<0)
                for(int axis=0;axis<3;++axis)
                    target[axis]+=fixedPoint(s32(randomNumber()&0x3FFFF)-0x1FFFF);
            d.travel=s16((randomNumber()&0xF)+9);
        }
        emit(source,target,d);
    }
}

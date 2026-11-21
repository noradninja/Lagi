// Recovered SH motion/timing/impact contracts, alongside unchanged A3 cases.
static void testNativeHomingProjectiles()
{
    const auto raw=[](const sVec3_FP& v,int axis){return v[axis].m_value;};
    // Negative odd coordinates distinguish native divide-toward-zero contact
    // from arithmetic-shift midpoint. Nonidentity inverse proves space handoff.
    sLagiBattleSphere a{},b{};
    a.position[0]=-5;b.position[0]=-4;
    a.position[1]=4;b.position[1]=5;
    a.radius=0x2000;b.radius=0x1000;
    cameraProperties2.m28[0].translation[0]=100;
    cameraProperties2.m28[0].translation[1]=-100;
    sVec3_FP impact;
    assert(lagiBattleSphereImpact(a,b,impact));
    assert(raw(impact,0)==96&&raw(impact,1)==-96);
    b.position[0]=100000;auto oldInverse=inverseTransforms;
    assert(!lagiBattleSphereImpact(a,b,impact)&&inverseTransforms==oldInverse);
    a.position={};b.position={};b.position[0]=0x3000;
    assert(lagiBattleSphereImpact(a,b,impact)); // touching is a hit
    b.position[0]=0x3010; // outside after native fixed-point square quantization
    assert(!lagiBattleSphereImpact(a,b,impact));
    cameraProperties2={};

    sVec3_FP position{},velocity{},acceleration{},target{};
    position[0]=-129;position[1]=129;position[2]=-300;
    velocity[0]=-9;velocity[1]=9;velocity[2]=-1;
    target[0]=-130;target[1]=130;target[2]=-299;
    lagiBattleHomingProjectileMotion(target,position,velocity,acceleration);
    assert(raw(acceleration,0)==1&&raw(acceleration,1)==-1&&raw(acceleration,2)==1);
    assert(raw(velocity,0)==-8&&raw(velocity,1)==8&&raw(velocity,2)==0);
    assert(raw(position,0)==-137&&raw(position,1)==137&&raw(position,2)==-300);

    // Delay uses the OLD counter; animation/scroll continue during the wait,
    // while motion/collision/lifetime do not. Only one RNG draw, no phase skip.
    for(u32 rng:{0u,1u,8u,9u}){
        nextRandom=rng;const unsigned beforeRandom=draws,beforeQuad=quadSteps;
        sVec3_FP source{},initial{};
        source[0]=1000000;source[1]=-1000000;initial[0]=-9;
        dragon.m8_position={};engine.m1A0_battleAutoScrollDelta={};
        engine.m1A0_battleAutoScrollDelta[2]=3;
        auto* t=static_cast<sLagiHomingBattleProjectile*>(
            lagiCreateNativeHomingBattleProjectile(&dragon,source,initial,{0x200},6,33,8));
        assert(draws==beforeRandom+1&&quadSteps==beforeQuad);
        assert(t->timer==75&&t->delay==s16(rng&9)&&t->damage==33&&t->damageType==8);
        const auto* saved=t->quad.frames;
        for(unsigned i=0;i<(rng&9);++i){
            lagiHomingBattleProjectileUpdate(t);
            assert(!t->control.finished&&t->timer==75&&raw(t->position,0)==1000000);
            assert(raw(t->position,2)==s32((i+1)*3)&&raw(t->velocity,0)==-9);
        }
        assert(t->delay==0);
        lagiHomingBattleProjectileUpdate(t);
        assert(t->delay==-1&&t->timer==74&&!t->control.finished);
        assert(raw(t->position,0)!=1000000&&quadSteps==beforeQuad+(rng&9)+1);
        lagiHomingBattleProjectileDraw(t);
        delete t;assert(saved&&!saved->empty()); // cache survives task deletion
    }
    // Lifetime also tests OLD signed value, not --timer: zero survives one
    // active update, then old -1 retires and clears drawing on the next.
    engine.m1A0_battleAutoScrollDelta={};nextRandom=0;
    sVec3_FP source{},initial{};source[0]=1000000;source[1]=-1000000;
    auto* t=static_cast<sLagiHomingBattleProjectile*>(
        lagiCreateNativeHomingBattleProjectile(&dragon,source,initial,{0x200},6,24,0));
    t->timer=0;lagiHomingBattleProjectileUpdate(t);
    assert(t->timer==-1&&!t->control.finished);
    lagiHomingBattleProjectileUpdate(t);
    assert(t->timer==-2&&t->control.finished&&!t->m_DrawMethod);
    delete t;

    for(bool lead:{false,true}){
        source={};source[0]=-5;dragon.m8_position={};dragon.m8_position[0]=-4;
        const unsigned beforeDamage=damages,beforeEmissions=unsigned(emissions.size());
        cameraProperties2.m28[0].translation[0]=100;
        t=static_cast<sLagiHomingBattleProjectile*>(
            lagiCreateNativeHomingBattleProjectile(&dragon,source,initial,{lead?0x300u:0x200u},6,lead?33:24,lead?8:0));
        lagiHomingBattleProjectileUpdate(t);
        assert(t->control.finished&&t->impacted&&!t->m_DrawMethod&&damages==beforeDamage+1);
        const auto& hit=damageCalls.back();
        assert(hit.damage==(lead?33:24)&&hit.flags==(lead?8:0)&&hit.type==2);
        assert(raw(hit.point,0)==96);
        for(int axis=0;axis<3;++axis)assert(raw(hit.impulse,axis)==0);
        assert(emissions.size()==beforeEmissions); // no invented trail/impact particles
        lagiHomingBattleProjectileUpdate(t);assert(damages==beforeDamage+1);
        delete t;
    }
    cameraProperties2={};nextRandom=2;
    std::puts("Native homing: signed motion, delay/expiry boundaries, contact space and one-hit damage PASS");
}

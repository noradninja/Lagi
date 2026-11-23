// FLD_A5 0605ED0C..0605EE84, 0605F070, 0605F188 and 0605F210.
// FLD_A5 060572F0, 060574BA and 060574CE/060575AC.
struct LagiOasisWormSprite : s_workAreaTemplateWithArg<LagiOasisWormSprite, std::pair<sVec3_FP, bool>>
{
    sAnimatedQuad quad;
    sVec3_FP position, velocity;
    sFieldModelRenderContext target;
    fixedPoint scale;
    fixedPoint ceiling;
    int age;
    u32 phase;
    bool trapped;

    static void Rescue(LagiOasisWormSprite* worm)
    {
        worm->target.m18_visibilityFlags |= 1;
        mainGameState.bitField[0x95] |= 8;
        worm->getTask()->markFinished();
    }

    static void Init(LagiOasisWormSprite* worm, std::pair<sVec3_FP, bool> spawn)
    {
        s_memoryAreaOutput memory;
        getMemoryArea(&memory, 3);
        static const auto drifting = initVdp1Quad(gFLD_A5->getSaturnPtr(0x06098E18));
        static const auto trapped = initVdp1Quad(gFLD_A5->getSaturnPtr(0x06098E34));
        particleInitSub(&worm->quad, (memory.m4_characterArea - 0x25C00000) >> 3,
            spawn.second ? &trapped : &drifting);
        worm->position = spawn.first;
        worm->velocity = {0, 0x800, 0};
        worm->scale = 0;
        worm->ceiling = getFieldTaskPtr()->m8_pSubFieldData->m338_pDragonTask->m140_maxY;
        worm->age = 0;
        worm->phase = 0;
        worm->trapped = spawn.second;
        if (worm->trapped)
            initFieldModelRenderContext(&worm->target, worm, reinterpret_cast<void*>(&Rescue),
                &worm->position, nullptr, 1, 0, -1, 0x63, 1);
    }

    static void Update(LagiOasisWormSprite* worm)
    {
        if (worm->trapped)
            updateFieldModelRenderContext(&worm->target);

        if (++worm->age < 20) {
            worm->scale += worm->trapped ? 0x147 : 0x51E;
        } else {
            worm->phase += 0x00AAAAAA;
            const auto wave = getSin((worm->phase >> 16) & 0xFFF);
            if (worm->trapped)
                worm->scale = (MTH_Mul(wave, 0x5555).asS32() + 0x6666) >> 2;
            else
                worm->scale = wave + 0x6666;
        }

        auto* field = static_cast<s_fieldSpecificData_A5*>(getFieldTaskPtr()->mC);
        const auto& wind = field->m4_pExitEntity->mC_direction;
        if (worm->trapped && worm->position.m4_Y <= 0)
            return;
        if (worm->trapped)
            worm->velocity.m4_Y -= 0xA3;
        const auto previousY = worm->position.m4_Y;
        for (int axis = 0; axis < 3; ++axis)
            worm->position[axis] += worm->velocity[axis] + (wind[axis].asS32() >> 2);
        if (worm->trapped && worm->position.m4_Y < 0)
            worm->position.m4_Y = 0;
        if (!worm->trapped && previousY > worm->ceiling)
            worm->getTask()->markFinished();
    }

    static void Draw(LagiOasisWormSprite* worm)
    {
        vdp1DrawQuadScaled(&worm->quad, &worm->position, worm->scale);
    }
};

struct LagiOasisSandPit : s_workAreaTemplateWithArg<LagiOasisSandPit, std::pair<sVec3_FP, unsigned>>
{
    s_memoryAreaOutput memory;
    sVec3_FP position;
    unsigned variant;
    int frame;
    fixedPoint scale;
    bool visible;
    int burstTimer;
    int cooldown;
    sVec3_FP targetPosition;
    sFieldModelRenderContext target;

    static void Trigger(LagiOasisSandPit* pit)
    {
        pit->frame = 0;
        pit->burstTimer = 15;
    }

    static void Init(LagiOasisSandPit* pit, std::pair<sVec3_FP, unsigned> placement)
    {
        getMemoryArea(&pit->memory, 3);
        pit->position = placement.first;
        pit->variant = placement.second;
        pit->frame = performModulo2(49, randomNumber());
        pit->scale = (randomNumber() & 0x7FFF) + 0x8000;
        pit->visible = false;
        pit->burstTimer = 0;
        pit->cooldown = 0;
        pit->targetPosition = pit->position;
        pit->targetPosition.m0_X += 0x1000;
        pit->targetPosition.m4_Y += MTH_Mul(0x24000, pit->scale);
        initFieldModelRenderContext(&pit->target, pit, reinterpret_cast<void*>(&Trigger),
            &pit->targetPosition, nullptr, 0, 0, -1, 0, 0);
    }

    static void Update(LagiOasisSandPit* pit)
    {
        pit->visible = !checkPositionVisibilityAgainstFarPlane(&pit->position);
        if (pit->cooldown > 0)
            --pit->cooldown;
        else if (!pit->burstTimer)
            updateFieldModelRenderContext(&pit->target);
        if (!pit->visible && !pit->burstTimer)
            return;
        if (pit->burstTimer > 1)
            --pit->burstTimer;
        if (!pit->burstTimer) {
            pit->frame = (pit->frame + 1) % 49;
            return;
        }
        if (++pit->frame == 29) {
            auto* field = static_cast<s_fieldSpecificData_A5*>(getFieldTaskPtr()->mC);
            const auto progress = mainGameState.readPackedBits(0x580, 8);
            const bool trapped = progress >= 5 && !field->m14_wormColorState0 &&
                !(mainGameState.bitField[0x95] & 8);
            if (trapped)
                field->m14_wormColorState0 = 1;
            if (progress < 200)
                mainGameState.setPackedBits(0x580, 8, progress + 1);
            auto position = pit->position;
            position.m4_Y += MTH_Mul(0x280000, pit->scale).asS32() >> 4;
            static const LagiOasisWormSprite::TypedTaskDefinition definition = {
                &LagiOasisWormSprite::Init, &LagiOasisWormSprite::Update,
                &LagiOasisWormSprite::Draw, nullptr,
            };
            createSubTaskWithArg<LagiOasisWormSprite>(pit, std::make_pair(position, trapped), &definition);
        } else if (pit->frame == 30) {
            pit->frame = 0;
            pit->burstTimer = 0;
            pit->cooldown = 480;
        }
    }

    static void Draw(LagiOasisSandPit* pit)
    {
        if (!pit->visible)
            return;
        const unsigned tableAddress = pit->burstTimer ? 0x0609C61C : 0x0609C610;
        const auto frameTable = readSaturnEA(gFLD_A5->getSaturnPtr(tableAddress + pit->variant * 4));
        const auto modelOffset = readSaturnU16(frameTable + pit->frame * 2);
        if (!modelOffset)
            return;
        const auto delta = cameraProperties2.m0_position - pit->position;
        pushCurrentMatrix();
        translateCurrentMatrix(&pit->position);
        rotateCurrentMatrixShiftedY(atan2_FP(delta.m0_X, delta.m8_Z) + 0x08000000);
        scaleCurrentMatrixRow0(pit->scale);
        scaleCurrentMatrixRow1(pit->scale);
        scaleCurrentMatrixRow2(pit->scale);
        if (gridCellDraw_GetDepthRange(pCurrentMatrix->m[2][3]) == 0)
            addObjectToDrawList(pit->memory.m0_mainMemoryBundle->get3DModel(modelOffset));
        popMatrix();
    }
};

static void lagiCreateOasisSandPit(s_visdibilityCellTask* cell,
    s_DataTable2Sub0& placement, int)
{
    const auto config = gFLD_A5->getSaturnPtr(0x0609C678);
    const unsigned progress = mainGameState.readPackedBits(0x580, 8);
    auto row = config;
    while (readSaturnU32(row) < progress)
        row += 8;
    auto* field = static_cast<s_fieldSpecificData_A5*>(getFieldTaskPtr()->mC);
    if (field->m18_wormColorState1++ % readSaturnU32(row + 4) == 0)
        return;

    unsigned variant = 0;
    if (placement.m0_function.m_offset == 0x0605EDBA)
        variant = 1;
    else if (placement.m0_function.m_offset == 0x0605EE34)
        variant = 2;
    static const LagiOasisSandPit::TypedTaskDefinition definition = {
        &LagiOasisSandPit::Init, &LagiOasisSandPit::Update, &LagiOasisSandPit::Draw, nullptr,
    };
    createSubTaskWithArg<LagiOasisSandPit>(cell,
        std::make_pair(placement.m4_position, variant), &definition);
}

static void lagiSetupGreenOasisField(s_DataTable3* environment, void(*startTasks)(p_workArea))
{
    for (unsigned factory : {0x0605ED66u, 0x0605EDBAu, 0x0605EE34u})
        lagiRegisterNativeFieldCellFactory(gFLD_A5, factory, lagiCreateOasisSandPit);
    setupFieldWithCellObjects(environment,
        readDataTable2(gFLD_A5->getSaturnPtr(0x0608FFA8)), startTasks);
}

// FLD_A5 0605F6F0 -> 0605F624. The item-box impact emits four copies
// of the original sand sprite with randomized horizontal position and velocity.
void lagiEmitOasisItemBoxDust(const sVec3_FP& impact)
{
    auto* field = static_cast<s_fieldSpecificData_A5*>(getFieldTaskPtr()->mC);
    if (!field || !field->m54_particlePool)
        return;
    static const auto quads = initVdp1Quad(gFLD_A5->getSaturnPtr(0x0608C508));
    for (int i = 0; i < 4; ++i) {
        const auto angle = (randomNumber() >> 16) & 0xFFF;
        sVec3_FP position = {
            impact.m0_X + MTH_Mul(0xA000, getCos(angle)),
            0,
            impact.m8_Z + MTH_Mul(0xA000, getSin(angle)),
        };
        sVec3_FP velocity = {MTH_Mul(0x800, getCos(angle)), 0, MTH_Mul(0x800, getSin(angle))};
        int lifetime = 5;
        sParticleSpawnConfig particle{};
        particle.m0_pPosition = &position;
        particle.m4_pVelocity = &velocity;
        particle.m8_pQuadData = &quads;
        particle.m14_updateFunc = [](sParticleSlot* slot) -> s32 {
            slot->m0_position += slot->mC_velocity;
            if (!(sGunShotTask_UpdateSub4(&slot->m2C_animQuad) & 2))
                return 0;
            return --*static_cast<int*>(slot->m20_heapData) <= 0;
        };
        particle.m18_heapSize = sizeof(lifetime);
        particle.m1C_heapData = &lifetime;
        spawnParticleInPool(field->m54_particlePool, &particle, 0);
    }
}

// FLD_A5 06057C0C, 0605782C, 060576F8. The large Baldor is a separate
// field task from the small worm sprite that comes out of a sand pit.
struct LagiOasisBaldor : s_workAreaTemplate<LagiOasisBaldor>
{
    s_memoryAreaOutput memory;
    std::shared_ptr<sMonsterBody> body;
    s_3dModel head, tail;
    sVec3_FP origin, position, rotation, shadowPosition;
    sFieldModelRenderContext target;
    std::vector<bool> sandReleased;
    fixedPoint radius, radiusVelocity;
    u32 orbit, timing;
    unsigned state, timer;

    static void Select(LagiOasisBaldor* worm)
    {
        worm->target.m18_visibilityFlags |= 1;
        playSystemSoundEffect(0x6B);
        worm->state = 2;
    }

    static void Init(LagiOasisBaldor* worm)
    {
        getMemoryArea(&worm->memory, 1);
        auto* bundle = worm->memory.m0_mainMemoryBundle;
        const auto config = gFLD_A5->getSaturnPtr(0x06099E78);
        const auto count = readSaturnU16(config + 2);
        worm->origin = worm->position = readSaturnVec3(config + 4);
        worm->rotation = {0x0C16C16C, 0, 0};
        worm->body.reset(monsterBody_create(worm, count));
        monsterBody_loadPartData(worm->body.get(), 0, {});
        worm->sandReleased.resize(count);
        for (unsigned i = 0; i < count; ++i) {
            auto& part = worm->body->m30_parts[i];
            part.m10_translation = readSaturnVec3(gFLD_A5->getSaturnPtr(0x06098F08));
            const u32 descriptor = i == 0 ? 0x06099DAC : i + 1 == count ? 0x06099DD4 : 0x06099DC0;
            monsterBodyPart_loadTemplate(&part, gFLD_A5->getSaturnPtr(descriptor));
            part.m44_springStiffness = {0xE00, 0xE00, 0xE00};
            part.m50_damping = 0x5000;
        }
        monsterBody_initPose(worm->body.get(), &worm->position, &worm->rotation);

        auto* headAnimation = bundle->getAnimation(0x2A0);
        auto* tailAnimation = bundle->getAnimation(0x29C);
        if (!init3DModelRawData(worm, &worm->head, 0, bundle, 0x3C, headAnimation,
                bundle->getStaticPose(0x268, headAnimation->m2_numBones), nullptr, nullptr) ||
            !init3DModelRawData(worm, &worm->tail, 0, bundle, 0x44, tailAnimation,
                bundle->getStaticPose(0x270, tailAnimation->m2_numBones), nullptr, nullptr)) {
            worm->getTask()->markFinished();
            return;
        }
        stepAnimation(&worm->head);
        stepAnimation(&worm->tail);
        worm->radius = 0x19000;
        worm->radiusVelocity = 0;
        worm->orbit = 0;
        worm->timing = 0;
        worm->state = 1;
        worm->timer = 0;
        initFieldModelRenderContext(&worm->target, worm, reinterpret_cast<void*>(&Select),
            &worm->position, nullptr, 1, 0, -1, 0, 0);
    }

    static void Update(LagiOasisBaldor* worm)
    {
        worm->timing += 0x400000;
        updateFieldModelRenderContext(&worm->target);
        if (worm->state == 2) {
            const bool alternate = (mainGameState.bitField[0xB] & 0x80) != 0;
            startFieldScript(alternate ? 16 : 12, alternate ? 0x5C5 : 0x5C1);
            worm->origin = worm->position;
            worm->origin.m0_X -= 0x78000;
            worm->radius = 0x78000;
            worm->orbit = 0;
            lagiSpawnA5WormBreachSand(&worm->position);
            worm->state = 3;
        }

        if (worm->state <= 1) {
            if (worm->state == 0) {
                if (worm->radiusVelocity > 0) {
                    worm->orbit -= 0x100000;
                    worm->rotation.m0_X = readSaturnFP(gFLD_A5->getSaturnPtr(
                        0x06098F14 + (randomNumber() & 3) * 4));
                }
                worm->timing -= 0x400000;
                worm->radius += worm->radiusVelocity;
                if (worm->radius >= 0x2D000) {
                    if (isWorldPositionOnScreen(&worm->position))
                        playSystemSoundEffect(0x6E);
                    worm->radiusVelocity = -0x2000;
                } else if (worm->radius <= 0x19000) {
                    worm->rotation.m0_X = 0x0C000000;
                    worm->state = 1;
                }
            } else if ((worm->orbit & 0x0FFFFFFF) == (worm->timing & 0x0FFFFFFF)) {
                worm->state = 0;
                worm->radius = 0x19000;
                worm->radiusVelocity = 0x800;
            }
            worm->orbit += 0x200000;
            const auto angle = (worm->orbit >> 16) & 0xFFF;
            const auto y = fixedPoint(static_cast<s32>(
                static_cast<int64_t>(0xD000) * worm->radius.asS32() / 0x73000) - 0xD000);
            worm->position = worm->origin + sVec3_FP{
                MTH_Mul(worm->radius, getSin(angle)), y, MTH_Mul(worm->radius, getCos(angle))};
            if ((worm->timer++ & 3) == 0)
                lagiSpawnA5WormSegmentSand(&worm->position);
            if (isWorldPositionOnScreen(&worm->position) && !(randomNumber() & 0xFF))
                playSystemSoundEffect(0x76);
        } else if (worm->state == 3) {
            const auto angle = (worm->orbit >> 16) & 0xFFF;
            const auto y = MTH_Mul(MTH_Mul(worm->radius, getSin(angle)), 0x6666);
            worm->position = worm->origin + sVec3_FP{MTH_Mul(worm->radius, getCos(angle)), y, 0};
            if (y < 0) {
                for (unsigned i = 0; i < worm->sandReleased.size(); ++i) {
                    auto& part = worm->body->m30_parts[i];
                    if (part.m4_worldPosition.m4_Y > 0x4000 && !worm->sandReleased[i]) {
                        lagiSpawnA5WormSegmentSand(&part.m4_worldPosition);
                        worm->sandReleased[i] = true;
                    }
                }
                if (worm->position.m4_Y <= -0x14000) {
                    lagiSpawnA5WormBreachSand(&worm->position);
                    worm->timer = 255;
                    worm->state = 4;
                    worm->m_DrawMethod = nullptr;
                }
            }
            worm->shadowPosition = {worm->position.m0_X, 0, worm->position.m8_Z};
            worm->rotation.m0_X += 0x200000;
            worm->orbit += 0x200000;
        } else if (worm->state == 4 && !--worm->timer) {
            playSystemSoundEffect(0x6E);
            mainGameState.bitField[0x95] |= 1;
            mainGameState.bitField[0xA3] |= 0x80;
            worm->getTask()->markFinished();
        }

        auto& body = *worm->body;
        body.m0_translation = worm->position;
        body.mC_rotation = body.m18_rotationTarget = worm->rotation;
        body.m24_update(body.m30_parts.data(), &body.m0_translation, &body.mC_rotation,
            &body.m18_rotationTarget);
        stepAnimation(&worm->head);
        stepAnimation(&worm->tail);
    }

    static void Draw(LagiOasisBaldor* worm)
    {
        auto* bundle = worm->memory.m0_mainMemoryBundle;
        auto& parts = worm->body->m30_parts;
        if (isWorldPositionOnScreen(&worm->position)) {
            const auto drawEnd = [](sMonsterBodyPart& part, s_3dModel& model) {
                pushCurrentMatrix();
                translateCurrentMatrix(&part.m4_worldPosition);
                rotateCurrentMatrixZYX(&part.m1C_rotation);
                model.m18_drawFunction(&model);
                popMatrix();
            };
            if (parts.front().m4_worldPosition.m4_Y >= -0x14000)
                drawEnd(parts.front(), worm->head);
            for (unsigned i = parts.size() - 2; i > 0; --i)
                if (parts[i].m4_worldPosition.m4_Y >= -0x14000)
                    worm->body->m2C_drawPart(bundle, &parts[i]);
            const auto& tail = parts.back();
            const int clearance = tail.m1C_rotation.m0_X >= 0 ? -0x1C000 : -0xC000;
            if (tail.m4_worldPosition.m4_Y >= clearance)
                drawEnd(parts.back(), worm->tail);
        }
        if (worm->state >= 3) {
            pushCurrentMatrix();
            translateCurrentMatrix(&worm->shadowPosition);
            rotateCurrentMatrixShiftedX(0x4000000);
            addObjectToDrawList(bundle->get3DModel(0xB4));
            popMatrix();
        }
    }
};

void createA5_baldorWormBody(p_workArea parent)
{
    if (mainGameState.bitField[0x95] & 1)
        return;
    static const LagiOasisBaldor::TypedTaskDefinition definition = {
        &LagiOasisBaldor::Init, &LagiOasisBaldor::Update, &LagiOasisBaldor::Draw, nullptr,
    };
    createSubTask<LagiOasisBaldor>(parent, &definition);
}

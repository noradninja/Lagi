// Original FLD_A5 0605A26C calls orbit camera 0607272E, not 060720C6.
static void lagiStartOasisDiscoveryOrbitCamera(sA5SandfallEntity* actor,
    const sSaturnPtr& data, s32 callback, sVec3_FP* target, u32 flags)
{
    auto& frames = actor->lagiDiscoveryOrbitFrames;
    frames.clear();
    for (auto entry=data; readSaturnU32(entry); entry+=0x24) {
        frames.push_back({readSaturnU32(entry), readSaturnU32(entry+4),
            readSaturnU32(entry+8), readSaturnU32(entry+12), readSaturnU32(entry+16),
            readSaturnU32(entry+20), readSaturnU32(entry+24), readSaturnU32(entry+28),
            readSaturnU32(entry+32)});
    }
    frames.push_back({});
    cutsceneTaskInitSub3(actor, frames, callback, target, flags);
}

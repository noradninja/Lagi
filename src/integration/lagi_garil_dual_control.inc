u32& lagiA5ActorFlags(p_workArea actor);
void battleEngine_displayAttackName(int,int,int);
static bool lagiGarilDualTimerAfter(s16& timer,s16 threshold) {
    const s16 old=timer;const u16 bits=u16(u32(s32(old))+1);
    std::memcpy(&timer,&bits,2);return old>threshold;
}

// Native FLD_A3 0605C088..0605C09C: one RNG draw selects one family.
template<class T> static void lagiWaterfallSmallVariantEmit(T* task)
{
    if (randomNumber() & 1)
        waterfallSpawnDroplet(task);
    else
        waterfallSpawnRisingMistVariant(task);
}

// Native 0605C21C selects 0605C18E for model table 06091CF8 (type 2).
// Types 3/4 retain 0605C12A. These are factory choices, not scene heuristics.
template<class T> static void lagiWaterfallLargeEmit(T* task, bool alternate)
{
    if (alternate) {
        // 0605C1C4..0605C1D2: no falling family, bit-0 gate, spread 80000.
        if (randomNumber() & 1)
            waterfallLargeSpawnSplash(task, 0x80000);
    } else {
        // 0605C162..0605C17C: periodic falling + bit-1:0 splash gate.
        if ((task->m10_frameCounter & 7) == 0)
            waterfallLargeSpawnFalling(task);
        if ((randomNumber() & 3) == 0)
            waterfallLargeSpawnSplash(task, 0x40000);
    }
}

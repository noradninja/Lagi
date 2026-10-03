#include "lagi/lagi_compat.h"
#include "heap.h"
#include "task.h"
#include "VDP1.h"
#include "VDP2.h"
#include "common.h"

// Upstream common.cpp owns the Saturn memory read helpers.
// Azel declares this helper in common.h but currently provides no definition,
// so the Vita host supplies only this missing engine utility.
sVec2_S16 readSaturnVec2_S16(sSaturnPtr ptr)
{
    sVec2_S16 out{};
    out[0] = readSaturnS16(ptr);
    out[1] = readSaturnS16(ptr + 2);
    return out;
}

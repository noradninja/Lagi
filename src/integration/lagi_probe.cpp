// Compile-only probe for Azel's foundational engine types on VitaSDK.
//
// Upstream PDS.h establishes this dependency order but also imports the
// desktop renderer/audio/debug stack. Reproduce only the portable core here.

#define SHIPPING_BUILD 1

#include "lagi/lagi_compat.h"
#include "heap.h"
#include "task.h"
#include "VDP1.h"
#include "VDP2.h"
#include "common.h"

namespace lagi::azel_probe {

static_assert(sizeof(u8) == 1, "Azel u8 must remain 8-bit");
static_assert(sizeof(u16) == 2, "Azel u16 must remain 16-bit");
static_assert(sizeof(u32) == 4, "Azel u32 must remain 32-bit");
static_assert(sizeof(s32) == 4, "Azel s32 must remain 32-bit");
static_assert(sizeof(fixedPoint) == 4, "Azel fixedPoint must remain 32-bit");

void compile_probe()
{
    fixedPoint zero(0);
    sVec3_FP origin;
    origin.zeroize();

    s_task task;
    sVdp2Controls controls{};

    (void)zero;
    (void)origin;
    (void)task;
    (void)controls;
}

} // namespace lagi::azel_probe

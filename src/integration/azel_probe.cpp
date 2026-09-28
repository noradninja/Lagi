// Compile-only probe for Azel's foundational engine types on VitaSDK.
//
// Do not include upstream PDS.h here: it is Azel's desktop umbrella/PCH and
// imports SDL3, BGFX, SoLoud, ImGui and Tracy. Lagi supplies the portable
// definitions those core headers actually require.

#include "lagi/azel_compat.h"
#include "common.h"
#include "task.h"

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
    (void)zero;
    (void)origin;
}

} // namespace lagi::azel_probe

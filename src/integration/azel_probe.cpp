// Compile-only probe for Azel's foundational engine types on VitaSDK.
// Keep this intentionally small: failures here identify language/header
// portability issues before desktop renderer/audio dependencies enter the build.

#include "common.h"
#include "fixedPoint.h"
#include "task.h"

namespace lagi::azel_probe {

static_assert(sizeof(u8) == 1, "Azel u8 must remain 8-bit");
static_assert(sizeof(u16) == 2, "Azel u16 must remain 16-bit");
static_assert(sizeof(u32) == 4, "Azel u32 must remain 32-bit");

void compile_probe()
{
    fixedPoint zero(0);
    (void)zero;
}

} // namespace lagi::azel_probe

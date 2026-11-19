// BTL_A3 06068BD0 projects target+4: the hotspot pointer is already
// in battle-camera/view space. The source sprite commands previously
// supplied a Saturn bucket key but no Neptune view-Z anchor.
// Scope only the three native target-indicator emitters; do not infer
// world depth for unrelated HUD/radar/menus from their sprite address.
namespace lagi {
template <typename Cursor>
struct WeakCursorWorldDepthScope
{
    Cursor& cursor;
    Cursor begin;
    float z;
    bool valid;

    template <typename Position>
    WeakCursorWorldDepthScope(Cursor& current, const Position* position)
        : cursor(current), begin(current),
          z(position ? static_cast<float>((*position)[2].asS32()) / 65536.0f : 0.0f),
          valid(position != nullptr)
    {
    }

    ~WeakCursorWorldDepthScope()
    {
        if (!valid || z <= 0.0f)
            return;
        const float viewZ[4] = {z, z, z, z};
        for (auto it = begin; it != cursor; ++it)
            lagiRecordVdp1WorldDepth(&*it, viewZ);
    }
};
} // namespace lagi

// Original A3 and A5 tasks draw the ordinary cursor independently of the
// angle-dependent damage multiplier. A bounded producer trace is needed to
// distinguish a wrong authored weak state from an incorrectly projected or
// occluded hotspot. This changes neither draws nor battle state.
// This .inc is included exclusively by the native battle CMake adapter.
// Keep the 32-sample probe available in Rider Release builds as well.
#if defined(LAGI_PLATFORM_VITA)
#include "lagi/platform.h"
#endif
namespace lagi {
template <typename Targetable>
inline void traceWeakCursor(const Targetable* target, unsigned quadrant,
                            unsigned cursorKind)
{
#if defined(LAGI_PLATFORM_VITA)
    if (!target || !target->m4_pPosition || quadrant >= 4 || cursorKind >= 2)
        return;
    // At most 32 entries across the entire run. One for each kind,
    // quadrant, coefficient class and laser-selection state.
    static unsigned char seen[2][4][2][2]{};
    static unsigned total = 0;
    const unsigned weak = target->m60 > 10 ? 1u : 0u;
    const unsigned laser = (target->m50_flags & 0x20000u) ? 1u : 0u;
    if (seen[cursorKind][quadrant][weak][laser] || total >= 32)
        return;
    seen[cursorKind][quadrant][weak][laser] = 1;
    ++total;
    const auto& pos = *target->m4_pPosition;
    lagi::platform::logging::writef(
        "[WeakCursorNative] kind=%u q=%u target=%p flags=%08X index=%d"
        " coeff=%d laser=%u cameraZ=%d rawXY=(%d,%d) sample=%u/32\n",
        cursorKind, quadrant, static_cast<const void*>(target),
        static_cast<unsigned>(target->m50_flags), static_cast<int>(target->m5A),
        static_cast<int>(target->m60), laser,
        static_cast<int>(pos[2].asS32()),
        static_cast<int>(pos[0].asS32()),
        static_cast<int>(pos[1].asS32()), total);
#else
    (void)target;
    (void)quadrant;
    (void)cursorKind;
#endif
}
} // namespace lagi

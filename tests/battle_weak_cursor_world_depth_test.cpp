#include <cassert>
#include <cstdint>

struct Fixed {
    std::int32_t raw;
    std::int32_t asS32() const { return raw; }
};
struct Position {
    Fixed xyz[3];
    Fixed operator[](int i) const { return xyz[i]; }
};
struct Command { int id; };
static const Command* last[8]{};
static float depth[8]{};
static int notifications = 0;
void lagiRecordVdp1WorldDepth(const void* command, const float z[4])
{
    assert(z[0] == z[1] && z[1] == z[2] && z[2] == z[3]);
    last[notifications] = static_cast<const Command*>(command);
    depth[notifications] = z[0];
    ++notifications;
}
#include "../src/integration/lagi_weak_cursor_world_depth.inc"

int main()
{
    Command commands[5]{};
    Command* cursor = commands;
    Position front{{{0}, {0}, {12 * 65536}}};
    {
        lagi::WeakCursorWorldDepthScope scope(cursor, &front);
        cursor += 3;
    }
    assert(notifications == 3);
    for (int i = 0; i < 3; ++i)
    {
        assert(last[i] == &commands[i]);
        assert(depth[i] == 12.0f);
    }
    // No command emission means no metadata, including early-return draws.
    { lagi::WeakCursorWorldDepthScope scope(cursor, &front); }
    assert(notifications == 3);
    // Null or behind-camera targets must not emit invalid world depths.
    {
        lagi::WeakCursorWorldDepthScope scope(cursor, static_cast<Position*>(nullptr));
        ++cursor;
    }
    Position behind{{{0}, {0}, {-65536}}};
    {
        lagi::WeakCursorWorldDepthScope scope(cursor, &behind);
        ++cursor;
    }
    assert(notifications == 3);
}

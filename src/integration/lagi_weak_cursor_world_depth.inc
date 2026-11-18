// BTL_A3 06068BD0 projects target+4: the hotspot pointer is already
// in battle-camera/view space. The source sprite commands previously
// supplied a Saturn bucket key but no Neptune view-Z anchor.
// Scope only the three native target-indicator emitters; do not infer
// world depth for unrelated HUD/radar/menus from their sprite address.
namespace lagi {
template <typename Command>
struct WeakCursorWorldDepthScope
{
    Command*& cursor;
    Command* begin;
    float z;
    bool valid;

    template <typename Position>
    WeakCursorWorldDepthScope(Command*& current, const Position* position)
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
        for (Command* command = begin; command < cursor; ++command)
            lagiRecordVdp1WorldDepth(command, viewZ);
    }
};
} // namespace lagi

#pragma once

namespace lagi {
// Module-manager modes: 1/2 town, 3 field, 4 direct battle. Ordinary field
// encounters retain mode 3 and publish the battle grid's authored view.
constexpr bool native_scene_mode(int mode)
{
    return mode>=1 && mode<=4;
}
constexpr bool native_scene_depth_mode(int mode)
{
    return mode==3 || mode==4;
}
}

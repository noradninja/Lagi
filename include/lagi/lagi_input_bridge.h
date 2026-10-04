#pragma once

namespace lagi::input_bridge {

// Publish the current Vita controls into Azel's pending Saturn 3D-pad state
// and advance Azel's input edge/held-state bookkeeping.
void sync_to_azel();

} // namespace lagi::input_bridge

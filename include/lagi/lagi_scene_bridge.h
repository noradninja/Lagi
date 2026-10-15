#pragma once

#include <cstdint>

namespace lagi::scene_bridge {

// Snapshot whatever native Azel scene state is currently required by the
// platform renderer. This dispatches to mode-specific adapters without making
// the runtime host own those game modes.
void sync_presentation_state();

// Read through the real Azel layout while each model is submitted.
void capture_local_coordinates(std::int16_t& x, std::int16_t& y);

} // namespace lagi::scene_bridge

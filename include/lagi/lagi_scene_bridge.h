#pragma once

namespace lagi::scene_bridge {

// Snapshot whatever native Azel scene state is currently required by the
// platform renderer. This dispatches to mode-specific adapters without making
// the runtime host own those game modes.
void sync_presentation_state();

} // namespace lagi::scene_bridge

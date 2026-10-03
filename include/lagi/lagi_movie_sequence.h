#pragma once

namespace lagi::azel {

// Direct boot bypasses Azel's module-manager task, so this adapter restores
// only the missing module transition boundary. The town script still chooses
// the next game status and Azel's original post-movie routine chooses what
// follows playback.
bool init_movie_sequence_adapter();

// Returns true while the movie adapter owns the current game frame. Callers
// must not advance or publish the town task graph for that frame.
bool service_movie_sequence_adapter();

} // namespace lagi::azel

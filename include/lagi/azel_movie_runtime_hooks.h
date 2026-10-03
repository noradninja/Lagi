#pragma once

#include <cstdint>

// Platform-service hooks used only by the generated Vita build copy of
// upstream Azel movie.cpp. Azel continues to own the movie task/state machine,
// filename/timing tables, sequencing, fades, skip behavior, and completion.
// These hooks replace only Saturn/desktop stream/decode/audio presentation.
void lagiAzelMovieStreamOpen(const char* cpkFileName);
void lagiAzelMovieStreamClose();
std::uint32_t lagiAzelMovieLastUpdate();

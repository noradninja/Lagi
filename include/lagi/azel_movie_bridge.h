#pragma once

#include <cstdint>

namespace lagi::azel {

// Thin service boundary for the original Azel movie task. These calls never
// choose a movie or a scene transition; the game task remains the owner.
bool movie_backend_open(const char* path);
void movie_backend_update(std::uint64_t elapsedMicroseconds);
void movie_backend_close();
bool movie_backend_active();
bool movie_backend_finished();
std::uint64_t movie_backend_pts();
const char* movie_backend_error();

} // namespace lagi::azel

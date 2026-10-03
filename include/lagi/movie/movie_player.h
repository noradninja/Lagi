#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "lagi/movie/cinepak_decoder.h"

namespace lagi::movie {

enum class PlaybackState : std::uint8_t {
    Closed,
    Playing,
    Finished,
    Failed,
};

// Azel owns this object and decides when to open, update, and close it.
// The player supplies only demux/decode/timing/platform services.
class MoviePlayer {
public:
    MoviePlayer();
    explicit MoviePlayer(std::unique_ptr<CinepakDecoder> decoder);
    ~MoviePlayer();

    MoviePlayer(const MoviePlayer&) = delete;
    MoviePlayer& operator=(const MoviePlayer&) = delete;

    bool open(const char* path);
    void update(std::uint64_t elapsedMicroseconds);
    void close();

    PlaybackState state() const;
    bool active() const { return state() == PlaybackState::Playing; }
    bool finished() const { return state() == PlaybackState::Finished; }
    std::uint64_t current_video_pts() const;
    std::uint32_t video_width() const;
    std::uint32_t video_height() const;
    const std::string& last_error() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace lagi::movie

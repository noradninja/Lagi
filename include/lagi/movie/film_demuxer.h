#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace lagi::movie {

enum class FilmStream : std::uint8_t {
    Video,
    Audio,
};

enum class FilmAudioCodec : std::uint8_t {
    None,
    PcmS8Planar,
    PcmS16BePlanar,
    Adx,
};

struct FilmVideoInfo {
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::uint32_t clockRate = 0;
};

struct FilmAudioInfo {
    FilmAudioCodec codec = FilmAudioCodec::None;
    std::uint32_t sampleRate = 0;
    std::uint8_t channels = 0;
    std::uint8_t bitsPerSample = 0;
};

struct FilmSample {
    FilmStream stream = FilmStream::Video;
    std::uint64_t offset = 0;
    std::uint32_t size = 0;
    std::uint64_t pts = 0;
    std::uint64_t duration = 0;
    bool keyframe = false;
};

struct FilmDataSource {
    void* context = nullptr;
    std::uint64_t size = 0;
    bool (*readAt)(
        void* context,
        std::uint64_t offset,
        void* destination,
        std::size_t bytes) = nullptr;
    void (*close)(void* context) = nullptr;

    bool valid() const { return context && readAt && close; }
};

class FilmDemuxer {
public:
    FilmDemuxer() = default;
    ~FilmDemuxer();

    FilmDemuxer(const FilmDemuxer&) = delete;
    FilmDemuxer& operator=(const FilmDemuxer&) = delete;

    bool open(const char* path);
    bool open(FilmDataSource source);
    void close();

    bool is_open() const { return source_.valid(); }
    const FilmVideoInfo& video_info() const { return videoInfo_; }
    const FilmAudioInfo& audio_info() const { return audioInfo_; }
    const std::vector<FilmSample>& samples() const { return samples_; }
    const std::string& last_error() const { return lastError_; }

    bool read_sample(std::size_t index, std::vector<std::uint8_t>& destination);

private:
    bool fail(const char* message);

    FilmDataSource source_{};
    std::uint64_t fileSize_ = 0;
    FilmVideoInfo videoInfo_{};
    FilmAudioInfo audioInfo_{};
    std::vector<FilmSample> samples_;
    std::string lastError_;
};

} // namespace lagi::movie

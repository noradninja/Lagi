#include "lagi/movie/film_demuxer.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <limits>

namespace lagi::movie {
namespace {

std::uint16_t read_be16(const std::uint8_t* p)
{
    return static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(p[0]) << 8) |
        static_cast<std::uint16_t>(p[1]));
}

std::uint32_t read_be32(const std::uint8_t* p)
{
    return (static_cast<std::uint32_t>(p[0]) << 24) |
           (static_cast<std::uint32_t>(p[1]) << 16) |
           (static_cast<std::uint32_t>(p[2]) << 8) |
           static_cast<std::uint32_t>(p[3]);
}

bool tag_is(const std::uint8_t* p, const char (&tag)[5])
{
    return p[0] == static_cast<std::uint8_t>(tag[0]) &&
           p[1] == static_cast<std::uint8_t>(tag[1]) &&
           p[2] == static_cast<std::uint8_t>(tag[2]) &&
           p[3] == static_cast<std::uint8_t>(tag[3]);
}

bool stdio_read_at(
    void* context,
    std::uint64_t offset,
    void* destination,
    std::size_t bytes)
{
    auto* file = static_cast<std::FILE*>(context);
    if (!file || offset > static_cast<std::uint64_t>(std::numeric_limits<long>::max()) ||
        std::fseek(file, static_cast<long>(offset), SEEK_SET) != 0)
        return false;
    return !bytes || std::fread(destination, 1, bytes, file) == bytes;
}

void stdio_close(void* context)
{
    if (context)
        std::fclose(static_cast<std::FILE*>(context));
}

} // namespace

FilmDemuxer::~FilmDemuxer()
{
    close();
}

bool FilmDemuxer::fail(const char* message)
{
    lastError_ = message ? message : "unknown FILM error";
    close();
    return false;
}

void FilmDemuxer::close()
{
    if (source_.valid())
        source_.close(source_.context);
    source_ = {};
    fileSize_ = 0;
    videoInfo_ = {};
    audioInfo_ = {};
    samples_.clear();
}

bool FilmDemuxer::open(const char* path)
{
    if (!path || !path[0])
        return fail("FILM path is empty");

    std::FILE* file = std::fopen(path, "rb");
    if (!file)
        return fail("unable to open FILM file");

    if (std::fseek(file, 0, SEEK_END) != 0) {
        std::fclose(file);
        return fail("unable to measure FILM file");
    }
    const long measuredSize = std::ftell(file);
    if (measuredSize < 0) {
        std::fclose(file);
        return fail("unable to measure FILM file");
    }

    FilmDataSource source{};
    source.context = file;
    source.size = static_cast<std::uint64_t>(measuredSize);
    source.readAt = stdio_read_at;
    source.close = stdio_close;
    return open(source);
}

bool FilmDemuxer::open(FilmDataSource source)
{
    close();
    lastError_.clear();
    if (!source.valid() || !source.size) {
        if (source.valid())
            source.close(source.context);
        return fail("FILM data source is invalid");
    }
    source_ = source;
    fileSize_ = source.size;
    std::uint64_t cursor = 0;
    auto read_next = [&](void* destination, std::size_t bytes) {
        if (bytes > fileSize_ - std::min(cursor, fileSize_))
            return false;
        const bool ok = source_.readAt(
            source_.context, cursor, destination, bytes);
        if (ok)
            cursor += bytes;
        return ok;
    };

    std::array<std::uint8_t, 16> header{};
    if (!read_next(header.data(), header.size()) ||
        !tag_is(header.data(), "FILM"))
        return fail("invalid FILM header");

    const std::uint32_t dataOffset = read_be32(header.data() + 4);
    const std::uint32_t version = read_be32(header.data() + 8);
    if (dataOffset < 36 || dataOffset > fileSize_)
        return fail("FILM data offset is outside the file");

    const std::size_t fdscBytes = version == 0 ? 20u : 32u;
    std::array<std::uint8_t, 32> fdsc{};
    if (!read_next(fdsc.data(), fdscBytes) ||
        !tag_is(fdsc.data(), "FDSC"))
        return fail("FILM FDSC chunk is missing");

    if (tag_is(fdsc.data() + 8, "cvid")) {
        videoInfo_.height = read_be32(fdsc.data() + 12);
        videoInfo_.width = read_be32(fdsc.data() + 16);
        if (!videoInfo_.width || !videoInfo_.height ||
            videoInfo_.width > 4096 || videoInfo_.height > 4096)
            return fail("FILM Cinepak dimensions are invalid");
    } else if (!tag_is(fdsc.data() + 8, "raw ")) {
        return fail("FILM video codec is not Cinepak");
    } else {
        return fail("raw FILM video is not supported by Phase 1");
    }

    if (version == 0) {
        audioInfo_.codec = FilmAudioCodec::PcmS8Planar;
        audioInfo_.sampleRate = 22050;
        audioInfo_.channels = 1;
        audioInfo_.bitsPerSample = 8;
    } else {
        audioInfo_.sampleRate = read_be16(fdsc.data() + 24);
        audioInfo_.channels = fdsc[21];
        audioInfo_.bitsPerSample = fdsc[22];
        const std::uint8_t compression = fdsc[23];

        if (!audioInfo_.channels) {
            audioInfo_.codec = FilmAudioCodec::None;
        } else if (compression == 2) {
            audioInfo_.codec = FilmAudioCodec::Adx;
        } else if (audioInfo_.bitsPerSample == 8) {
            audioInfo_.codec = FilmAudioCodec::PcmS8Planar;
        } else if (audioInfo_.bitsPerSample == 16) {
            audioInfo_.codec = FilmAudioCodec::PcmS16BePlanar;
        } else {
            return fail("FILM PCM bit depth is unsupported");
        }

        if (audioInfo_.channels > 2)
            return fail("FILM audio has more than two channels");
        if (audioInfo_.channels && !audioInfo_.sampleRate)
            return fail("FILM audio sample rate is zero");
    }

    std::array<std::uint8_t, 16> stab{};
    if (!read_next(stab.data(), stab.size()) ||
        !tag_is(stab.data(), "STAB"))
        return fail("FILM STAB chunk is missing");

    videoInfo_.clockRate = read_be32(stab.data() + 8);
    const std::uint32_t sampleCount = read_be32(stab.data() + 12);
    if (!videoInfo_.clockRate)
        return fail("FILM video clock is zero");

    const std::uint64_t tableStart = cursor;
    const std::uint64_t tableBytes = static_cast<std::uint64_t>(sampleCount) * 16u;
    if (tableStart > dataOffset || tableBytes > dataOffset - tableStart)
        return fail("FILM sample table exceeds the header");

    samples_.reserve(sampleCount);
    std::uint64_t audioFrameCounter = 0;
    std::array<std::uint8_t, 16> entry{};

    for (std::uint32_t i = 0; i < sampleCount; ++i) {
        if (!read_next(entry.data(), entry.size()))
            return fail("FILM sample table is truncated");

        FilmSample sample{};
        const std::uint32_t relativeOffset = read_be32(entry.data());
        sample.offset = static_cast<std::uint64_t>(dataOffset) + relativeOffset;
        sample.size = read_be32(entry.data() + 4);
        const std::uint32_t info1 = read_be32(entry.data() + 8);

        if (sample.offset > fileSize_ || sample.size > fileSize_ - sample.offset)
            return fail("FILM sample points outside the file");

        if (info1 == 0xFFFFFFFFu) {
            if (audioInfo_.codec == FilmAudioCodec::None)
                return fail("FILM contains audio samples without an audio format");

            sample.stream = FilmStream::Audio;
            sample.pts = audioFrameCounter;
            sample.keyframe = true;

            if (audioInfo_.codec == FilmAudioCodec::Adx) {
                if (!audioInfo_.channels)
                    return fail("FILM ADX channel count is zero");
                audioFrameCounter +=
                    (static_cast<std::uint64_t>(sample.size) * 32u) /
                    (18u * audioInfo_.channels);
            } else {
                const std::uint32_t bytesPerFrame =
                    audioInfo_.channels * (audioInfo_.bitsPerSample / 8u);
                if (!bytesPerFrame || sample.size % bytesPerFrame != 0)
                    return fail("FILM PCM sample is not frame-aligned");
                audioFrameCounter += sample.size / bytesPerFrame;
            }
        } else {
            sample.stream = FilmStream::Video;
            sample.pts = info1 & 0x7FFFFFFFu;
            sample.keyframe = (info1 & 0x80000000u) == 0;
        }

        samples_.push_back(sample);
    }

    for (std::size_t i = 0; i < samples_.size(); ++i) {
        for (std::size_t next = i + 1; next < samples_.size(); ++next) {
            if (samples_[next].stream == samples_[i].stream) {
                if (samples_[next].pts >= samples_[i].pts)
                    samples_[i].duration = samples_[next].pts - samples_[i].pts;
                break;
            }
        }
    }

    const bool hasVideo = std::any_of(
        samples_.begin(), samples_.end(), [](const FilmSample& sample) {
            return sample.stream == FilmStream::Video && sample.size != 0;
        });
    if (!hasVideo)
        return fail("FILM contains no video samples");

    return true;
}

bool FilmDemuxer::read_sample(
    std::size_t index,
    std::vector<std::uint8_t>& destination)
{
    if (!source_.valid() || index >= samples_.size()) {
        lastError_ = "FILM sample index is invalid";
        return false;
    }

    const FilmSample& sample = samples_[index];
    destination.resize(sample.size);
    if (sample.size &&
        !source_.readAt(
            source_.context,
            sample.offset,
            destination.data(),
            destination.size())) {
        destination.clear();
        lastError_ = "FILM sample is truncated";
        return false;
    }
    return true;
}

} // namespace lagi::movie

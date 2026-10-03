#include "lagi/movie/movie_player.h"

#include "lagi/disc_image.h"
#include "lagi/movie/film_demuxer.h"
#include "lagi/platform.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace lagi::movie {
namespace {

struct DiscFilmContext {
    disc::FileHandle file{};
};

bool disc_film_read_at(
    void* context,
    std::uint64_t offset,
    void* destination,
    std::size_t bytes)
{
    auto* source = static_cast<DiscFilmContext*>(context);
    return source && disc::read_file_at(
        source->file, offset, destination, bytes);
}

void disc_film_close(void* context)
{
    auto* source = static_cast<DiscFilmContext*>(context);
    if (source) {
        disc::close_file(source->file);
        delete source;
    }
}

bool open_disc_film(FilmDemuxer& demuxer, const char* path)
{
    auto* context = new DiscFilmContext();
    if (!disc::open_file(path, context->file)) {
        delete context;
        return false;
    }

    FilmDataSource source{};
    source.context = context;
    source.size = context->file.size;
    source.readAt = disc_film_read_at;
    source.close = disc_film_close;
    return demuxer.open(source);
}

} // namespace

class MoviePlayer::Impl {
public:
    explicit Impl(std::unique_ptr<CinepakDecoder> suppliedDecoder)
        : decoder(std::move(suppliedDecoder))
    {
        if (!decoder)
            decoder = make_cpu_cinepak_decoder();
    }

    ~Impl()
    {
        close();
    }

    bool open(const char* path)
    {
        close();
        error.clear();
        if (!demuxer.open(path) && !open_disc_film(demuxer, path))
            return fail("unable to open FILM file from filesystem or mounted disc");

        const FilmVideoInfo& video = demuxer.video_info();
        if (!decoder || !decoder->configure(video.width, video.height))
            return fail(decoder ? decoder->last_error() : "Cinepak decoder is unavailable");

        const FilmAudioInfo& audio = demuxer.audio_info();
        if (audio.codec == FilmAudioCodec::Adx)
            return fail("FILM ADX audio is outside the 0.030-alpha PCM path");

        const auto& table = demuxer.samples();
        for (std::size_t i = 0; i < table.size(); ++i) {
            if (table[i].stream == FilmStream::Video)
                videoSamples.push_back(i);
            else
                audioSamples.push_back(i);
        }

        if (videoSamples.size() >= 2u) {
            const FilmSample& first = table[videoSamples.front()];
            const FilmSample& second = table[videoSamples[1]];
            const FilmSample& last = table[videoSamples.back()];
            const std::uint64_t spanTicks =
                last.pts >= first.pts ? last.pts - first.pts : 0u;
            const std::uint64_t intervalTicks =
                second.pts >= first.pts ? second.pts - first.pts : 0u;
            const std::uint64_t fpsMilli =
                spanTicks != 0u
                    ? (static_cast<std::uint64_t>(videoSamples.size() - 1u) *
                       static_cast<std::uint64_t>(video.clockRate) *
                       1000ull) / spanTicks
                    : 0u;
            platform::logging::writef(
                "[MovieTiming] videoSamples=%u clock=%u firstDelta=%llu "
                "avgFps=%llu.%03llu pts=%llu..%llu\n",
                static_cast<unsigned int>(videoSamples.size()),
                video.clockRate,
                static_cast<unsigned long long>(intervalTicks),
                static_cast<unsigned long long>(fpsMilli / 1000ull),
                static_cast<unsigned long long>(fpsMilli % 1000ull),
                static_cast<unsigned long long>(first.pts),
                static_cast<unsigned long long>(last.pts));
        }

        hasAudio = audio.codec == FilmAudioCodec::PcmS8Planar ||
                   audio.codec == FilmAudioCodec::PcmS16BePlanar;
        if (hasAudio &&
            !platform::audio::start_pcm_stream(audio.sampleRate, audio.channels))
            return fail("SceAudio could not open the movie PCM stream");

        state = PlaybackState::Playing;
        pump_audio();
        return state == PlaybackState::Playing;
    }

    void update(std::uint64_t elapsedMicroseconds)
    {
        if (state != PlaybackState::Playing)
            return;

        if (!hasAudio)
            fallbackClockUs += elapsedMicroseconds;
        pump_audio();
        if (state != PlaybackState::Playing)
            return;

        const FilmVideoInfo& video = demuxer.video_info();
        const std::uint64_t clockUs = hasAudio
            ? (platform::audio::played_pcm_frames() * 1000000ull) /
                demuxer.audio_info().sampleRate
            : fallbackClockUs;
        const std::uint64_t clockTicks =
            (clockUs * video.clockRate) / 1000000ull;

        bool decodedFrame = false;
        while (nextVideo < videoSamples.size()) {
            const std::size_t sampleIndex = videoSamples[nextVideo];
            const FilmSample& sample = demuxer.samples()[sampleIndex];
            if (sample.pts > clockTicks)
                break;

            if (!demuxer.read_sample(sampleIndex, sampleBytes)) {
                fail(demuxer.last_error());
                return;
            }
            if (!decoder->decode(sampleBytes.data(), sampleBytes.size())) {
                fail(decoder->last_error());
                return;
            }

            currentPts = sample.pts;
            ++nextVideo;
            decodedFrame = true;
        }

        // Decode every due interframe, but upload only the newest result.
        if (decodedFrame) {
            if (!decoder->present()) {
                fail(decoder->last_error());
                return;
            }
        }

        const bool videoDone = nextVideo >= videoSamples.size();
        const bool audioDone = !hasAudio || platform::audio::pcm_stream_drained();
        if (videoDone && audioDone)
            state = PlaybackState::Finished;
    }

    void close()
    {
        platform::audio::stop_pcm_stream();
        platform::renderer::movie_clear_frame();
        demuxer.close();
        videoSamples.clear();
        audioSamples.clear();
        sampleBytes.clear();
        pendingPcm.clear();
        pendingFrameOffset = 0;
        nextVideo = 0;
        nextAudio = 0;
        fallbackClockUs = 0;
        currentPts = 0;
        hasAudio = false;
        audioFinishSent = false;
        state = PlaybackState::Closed;
        error.clear();
    }

    bool fail(const std::string& message)
    {
        error = message.empty() ? "movie playback failed" : message;
        platform::audio::stop_pcm_stream();
        platform::renderer::movie_clear_frame();
        state = PlaybackState::Failed;
        return false;
    }

    bool convert_pcm_sample(std::size_t sampleIndex)
    {
        if (!demuxer.read_sample(sampleIndex, sampleBytes))
            return fail(demuxer.last_error());

        const FilmAudioInfo& audio = demuxer.audio_info();
        const std::size_t bytesPerSample = audio.bitsPerSample / 8u;
        const std::size_t bytesPerFrame = bytesPerSample * audio.channels;
        if (!bytesPerFrame || sampleBytes.size() % bytesPerFrame)
            return fail("FILM PCM sample is not frame-aligned");

        const std::size_t frameCount = sampleBytes.size() / bytesPerFrame;
        const std::size_t planeBytes = frameCount * bytesPerSample;
        pendingPcm.resize(frameCount * audio.channels);
        pendingFrameOffset = 0;

        for (std::size_t frame = 0; frame < frameCount; ++frame) {
            for (std::size_t channel = 0; channel < audio.channels; ++channel) {
                const std::size_t source =
                    channel * planeBytes + frame * bytesPerSample;
                std::int16_t value = 0;
                if (audio.codec == FilmAudioCodec::PcmS8Planar) {
                    value = static_cast<std::int16_t>(
                        static_cast<std::int8_t>(sampleBytes[source])) * 256;
                } else if (audio.codec == FilmAudioCodec::PcmS16BePlanar) {
                    const std::uint16_t encoded =
                        (static_cast<std::uint16_t>(sampleBytes[source]) << 8) |
                        static_cast<std::uint16_t>(sampleBytes[source + 1]);
                    value = static_cast<std::int16_t>(encoded);
                } else {
                    return fail("FILM audio is not PCM");
                }
                pendingPcm[frame * audio.channels + channel] = value;
            }
        }
        return true;
    }

    void pump_audio()
    {
        if (!hasAudio || state == PlaybackState::Failed)
            return;

        const std::size_t channels = demuxer.audio_info().channels;
        while (true) {
            if (pendingFrameOffset < pendingPcm.size() / channels) {
                const std::size_t totalFrames = pendingPcm.size() / channels;
                const std::size_t remaining = totalFrames - pendingFrameOffset;
                const std::size_t accepted = platform::audio::write_pcm_frames(
                    pendingPcm.data() + pendingFrameOffset * channels,
                    remaining);
                pendingFrameOffset += accepted;
                if (pendingFrameOffset < totalFrames)
                    return;
                pendingPcm.clear();
                pendingFrameOffset = 0;
            }

            if (nextAudio >= audioSamples.size()) {
                if (!audioFinishSent) {
                    platform::audio::finish_pcm_stream();
                    audioFinishSent = true;
                }
                return;
            }

            if (!convert_pcm_sample(audioSamples[nextAudio]))
                return;
            ++nextAudio;
        }
    }

    std::unique_ptr<CinepakDecoder> decoder;
    FilmDemuxer demuxer;
    std::vector<std::size_t> videoSamples;
    std::vector<std::size_t> audioSamples;
    std::vector<std::uint8_t> sampleBytes;
    std::vector<std::int16_t> pendingPcm;
    std::size_t pendingFrameOffset = 0;
    std::size_t nextVideo = 0;
    std::size_t nextAudio = 0;
    std::uint64_t fallbackClockUs = 0;
    std::uint64_t currentPts = 0;
    bool hasAudio = false;
    bool audioFinishSent = false;
    PlaybackState state = PlaybackState::Closed;
    std::string error;
};

MoviePlayer::MoviePlayer()
#if defined(LAGI_PLATFORM_VITA)
    : impl_(std::make_unique<Impl>(make_sgx_cinepak_decoder()))
#else
    : impl_(std::make_unique<Impl>(make_cpu_cinepak_decoder()))
#endif
{
}

MoviePlayer::MoviePlayer(std::unique_ptr<CinepakDecoder> decoder)
    : impl_(std::make_unique<Impl>(std::move(decoder)))
{
}

MoviePlayer::~MoviePlayer() = default;

bool MoviePlayer::open(const char* path)
{
    return impl_->open(path);
}

void MoviePlayer::update(std::uint64_t elapsedMicroseconds)
{
    impl_->update(elapsedMicroseconds);
}

void MoviePlayer::close()
{
    impl_->close();
}

PlaybackState MoviePlayer::state() const
{
    return impl_->state;
}

std::uint64_t MoviePlayer::current_video_pts() const
{
    return impl_->currentPts;
}

std::uint32_t MoviePlayer::video_width() const
{
    return impl_->demuxer.video_info().width;
}

std::uint32_t MoviePlayer::video_height() const
{
    return impl_->demuxer.video_info().height;
}

const std::string& MoviePlayer::last_error() const
{
    return impl_->error;
}

} // namespace lagi::movie

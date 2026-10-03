#include "lagi/movie/cinepak_decoder.h"
#include "lagi/movie/film_demuxer.h"

#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <vector>

namespace lagi::platform::renderer {
bool movie_present_frame(
    const std::uint32_t*, unsigned int, unsigned int, unsigned int)
{
    return true;
}
}

namespace {

void put_be16(std::vector<std::uint8_t>& data, std::size_t at, std::uint16_t value)
{
    data[at + 0] = static_cast<std::uint8_t>(value >> 8);
    data[at + 1] = static_cast<std::uint8_t>(value);
}

void put_be24(std::vector<std::uint8_t>& data, std::size_t at, std::uint32_t value)
{
    data[at + 0] = static_cast<std::uint8_t>(value >> 16);
    data[at + 1] = static_cast<std::uint8_t>(value >> 8);
    data[at + 2] = static_cast<std::uint8_t>(value);
}

void put_be32(std::vector<std::uint8_t>& data, std::size_t at, std::uint32_t value)
{
    data[at + 0] = static_cast<std::uint8_t>(value >> 24);
    data[at + 1] = static_cast<std::uint8_t>(value >> 16);
    data[at + 2] = static_cast<std::uint8_t>(value >> 8);
    data[at + 3] = static_cast<std::uint8_t>(value);
}

std::vector<std::uint8_t> make_frame()
{
    std::vector<std::uint8_t> frame(37, 0);
    frame[0] = 1;
    put_be24(frame, 1, static_cast<std::uint32_t>(frame.size()));
    put_be16(frame, 4, 4);
    put_be16(frame, 6, 4);
    put_be16(frame, 8, 1);

    frame[10] = 0x10;
    put_be24(frame, 11, 27);
    put_be16(frame, 14, 0);
    put_be16(frame, 16, 0);
    put_be16(frame, 18, 4);
    put_be16(frame, 20, 4);

    frame[22] = 0x22;
    put_be24(frame, 23, 10);
    frame[26] = 10;
    frame[27] = 20;
    frame[28] = 30;
    frame[29] = 40;
    frame[30] = 0;
    frame[31] = 0;

    frame[32] = 0x32;
    put_be24(frame, 33, 5);
    frame[36] = 0;
    return frame;
}

std::vector<std::uint8_t> make_v4_frame()
{
    std::vector<std::uint8_t> frame(44, 0);
    frame[0] = 1;
    put_be24(frame, 1, static_cast<std::uint32_t>(frame.size()));
    put_be16(frame, 4, 4);
    put_be16(frame, 6, 4);
    put_be16(frame, 8, 1);
    frame[10] = 0x10;
    put_be24(frame, 11, 34);
    put_be16(frame, 18, 4);
    put_be16(frame, 20, 4);

    frame[22] = 0x20;
    put_be24(frame, 23, 10);
    frame[26] = 1;
    frame[27] = 2;
    frame[28] = 3;
    frame[29] = 4;

    frame[32] = 0x30;
    put_be24(frame, 33, 12);
    put_be32(frame, 36, 0x80000000u);
    return frame;
}

std::vector<std::uint8_t> make_skipped_frame()
{
    std::vector<std::uint8_t> frame(30, 0);
    frame[0] = 1;
    put_be24(frame, 1, static_cast<std::uint32_t>(frame.size()));
    put_be16(frame, 4, 4);
    put_be16(frame, 6, 4);
    put_be16(frame, 8, 1);
    frame[10] = 0x11;
    put_be24(frame, 11, 20);
    put_be16(frame, 18, 4);
    put_be16(frame, 20, 4);
    frame[22] = 0x31;
    put_be24(frame, 23, 8);
    put_be32(frame, 26, 0);
    return frame;
}

std::vector<std::uint8_t> make_sega_skip_frame()
{
    std::vector<std::uint8_t> original = make_frame();
    std::vector<std::uint8_t> frame;
    frame.reserve(original.size() + 2);
    frame.insert(frame.end(), original.begin(), original.begin() + 10);
    frame.push_back(0);
    frame.push_back(0);
    frame.insert(frame.end(), original.begin() + 10, original.end());
    return frame;
}

std::vector<std::uint8_t> make_film()
{
    const std::vector<std::uint8_t> frame = make_frame();
    constexpr std::uint32_t dataOffset = 96;
    std::vector<std::uint8_t> film(dataOffset + frame.size() + 4, 0);
    film[0] = 'F'; film[1] = 'I'; film[2] = 'L'; film[3] = 'M';
    put_be32(film, 4, dataOffset);
    put_be32(film, 8, 1);

    film[16] = 'F'; film[17] = 'D'; film[18] = 'S'; film[19] = 'C';
    put_be32(film, 20, 32);
    film[24] = 'c'; film[25] = 'v'; film[26] = 'i'; film[27] = 'd';
    put_be32(film, 28, 4);
    put_be32(film, 32, 4);
    film[37] = 1;
    film[38] = 8;
    put_be16(film, 40, 22050);

    film[48] = 'S'; film[49] = 'T'; film[50] = 'A'; film[51] = 'B';
    put_be32(film, 52, 48);
    put_be32(film, 56, 30);
    put_be32(film, 60, 2);

    put_be32(film, 64, 0);
    put_be32(film, 68, static_cast<std::uint32_t>(frame.size()));
    put_be32(film, 72, 0);
    put_be32(film, 76, 0);

    put_be32(film, 80, static_cast<std::uint32_t>(frame.size()));
    put_be32(film, 84, 4);
    put_be32(film, 88, 0xFFFFFFFFu);
    put_be32(film, 92, 0);

    for (std::size_t i = 0; i < frame.size(); ++i)
        film[dataOffset + i] = frame[i];
    film[dataOffset + frame.size() + 0] = 0x80;
    film[dataOffset + frame.size() + 1] = 0x00;
    film[dataOffset + frame.size() + 2] = 0x7F;
    film[dataOffset + frame.size() + 3] = 0xFF;
    return film;
}

std::uint32_t gray(std::uint8_t value)
{
    return 0xFF000000u | value |
           (static_cast<std::uint32_t>(value) << 8) |
           (static_cast<std::uint32_t>(value) << 16);
}

} // namespace

int main()
{
    const char* path = "lagi_movie_core_test.cpk";
    const std::vector<std::uint8_t> filmBytes = make_film();
    std::FILE* output = std::fopen(path, "wb");
    assert(output);
    assert(std::fwrite(filmBytes.data(), 1, filmBytes.size(), output) == filmBytes.size());
    std::fclose(output);

    lagi::movie::FilmDemuxer demuxer;
    assert(demuxer.open(path));
    assert(demuxer.video_info().width == 4);
    assert(demuxer.video_info().height == 4);
    assert(demuxer.video_info().clockRate == 30);
    assert(demuxer.audio_info().codec == lagi::movie::FilmAudioCodec::PcmS8Planar);
    assert(demuxer.audio_info().sampleRate == 22050);
    assert(demuxer.samples().size() == 2);
    assert(demuxer.samples()[0].stream == lagi::movie::FilmStream::Video);
    assert(demuxer.samples()[1].stream == lagi::movie::FilmStream::Audio);

    std::vector<std::uint8_t> frameBytes;
    assert(demuxer.read_sample(0, frameBytes));
    auto decoder = lagi::movie::make_cpu_cinepak_decoder();
    assert(decoder->configure(4, 4));
    assert(decoder->decode(frameBytes.data(), frameBytes.size()));
    const lagi::movie::VideoFrame frame = decoder->frame();
    assert(frame.rgba && frame.width == 4 && frame.height == 4);

    const std::array<std::uint32_t, 16> expected = {
        gray(10), gray(10), gray(20), gray(20),
        gray(10), gray(10), gray(20), gray(20),
        gray(30), gray(30), gray(40), gray(40),
        gray(30), gray(30), gray(40), gray(40),
    };
    for (std::size_t i = 0; i < expected.size(); ++i)
        assert(frame.rgba[i] == expected[i]);
    assert(decoder->present());

    const std::vector<std::uint8_t> v4Bytes = make_v4_frame();
    assert(decoder->decode(v4Bytes.data(), v4Bytes.size()));
    const lagi::movie::VideoFrame v4Frame = decoder->frame();
    const std::array<std::uint32_t, 16> v4Expected = {
        gray(1), gray(2), gray(1), gray(2),
        gray(3), gray(4), gray(3), gray(4),
        gray(1), gray(2), gray(1), gray(2),
        gray(3), gray(4), gray(3), gray(4),
    };
    for (std::size_t i = 0; i < v4Expected.size(); ++i)
        assert(v4Frame.rgba[i] == v4Expected[i]);

    const std::vector<std::uint8_t> skippedBytes = make_skipped_frame();
    assert(decoder->decode(skippedBytes.data(), skippedBytes.size()));
    const lagi::movie::VideoFrame skippedFrame = decoder->frame();
    for (std::size_t i = 0; i < v4Expected.size(); ++i)
        assert(skippedFrame.rgba[i] == v4Expected[i]);

    auto segaDecoder = lagi::movie::make_cpu_cinepak_decoder();
    assert(segaDecoder->configure(4, 4));
    const std::vector<std::uint8_t> segaBytes = make_sega_skip_frame();
    assert(segaDecoder->decode(segaBytes.data(), segaBytes.size()));
    const lagi::movie::VideoFrame segaFrame = segaDecoder->frame();
    for (std::size_t i = 0; i < expected.size(); ++i)
        assert(segaFrame.rgba[i] == expected[i]);

    demuxer.close();
    std::remove(path);
    std::puts("movie core tests passed");
    return 0;
}

#include "lagi/movie/cinepak_decoder.h"
#include "lagi/platform.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <vector>

namespace lagi::movie {
namespace {

std::uint16_t read_be16(const std::uint8_t* p)
{
    return static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(p[0]) << 8) |
        static_cast<std::uint16_t>(p[1]));
}

std::uint32_t read_be24(const std::uint8_t* p)
{
    return (static_cast<std::uint32_t>(p[0]) << 16) |
           (static_cast<std::uint32_t>(p[1]) << 8) |
           static_cast<std::uint32_t>(p[2]);
}

std::uint32_t read_be32(const std::uint8_t* p)
{
    return (static_cast<std::uint32_t>(p[0]) << 24) |
           (static_cast<std::uint32_t>(p[1]) << 16) |
           (static_cast<std::uint32_t>(p[2]) << 8) |
           static_cast<std::uint32_t>(p[3]);
}

std::uint8_t clamp_byte(int value)
{
    return static_cast<std::uint8_t>(std::clamp(value, 0, 255));
}

std::uint32_t rgba_from_yuv(std::uint8_t y, std::int8_t u, std::int8_t v)
{
    const int r = static_cast<int>(y) + static_cast<int>(v) * 2;
    const int g = static_cast<int>(y) - static_cast<int>(u) / 2 - static_cast<int>(v);
    const int b = static_cast<int>(y) + static_cast<int>(u) * 2;
    return 0xFF000000u |
           static_cast<std::uint32_t>(clamp_byte(r)) |
           (static_cast<std::uint32_t>(clamp_byte(g)) << 8) |
           (static_cast<std::uint32_t>(clamp_byte(b)) << 16);
}

std::uint32_t rgba_from_gray(std::uint8_t value)
{
    return 0xFF000000u |
           static_cast<std::uint32_t>(value) |
           (static_cast<std::uint32_t>(value) << 8) |
           (static_cast<std::uint32_t>(value) << 16);
}

struct CodebookEntry {
    std::uint32_t rgba[4]{};
};

struct Strip {
    std::uint16_t x1 = 0;
    std::uint16_t y1 = 0;
    std::uint16_t x2 = 0;
    std::uint16_t y2 = 0;
    std::array<CodebookEntry, 256> v4{};
    std::array<CodebookEntry, 256> v1{};
};

class CpuCinepakDecoder final : public CinepakDecoder {
public:
    bool configure(std::uint32_t width, std::uint32_t height) override
    {
        error_.clear();
        if (!width || !height || width > 4096 || height > 4096)
            return fail("Cinepak dimensions are invalid");
        width_ = width;
        height_ = height;
        pixels_.assign(static_cast<std::size_t>(width) * height, 0xFF000000u);
        strips_ = {};
        segaSkipBytes_ = -1;
        return true;
    }

    bool decode(const std::uint8_t* data, std::size_t size) override
    {
        error_.clear();
        if (!width_ || !height_ || pixels_.empty())
            return fail("Cinepak decoder is not configured");
        if (!data || size < 10)
            return fail("Cinepak frame header is truncated");

        const std::uint32_t encodedSize = read_be24(data + 1);
        const std::uint16_t frameWidth = read_be16(data + 4);
        const std::uint16_t frameHeight = read_be16(data + 6);
        const std::uint16_t stripCount = read_be16(data + 8);
        if (encodedSize == 0 || encodedSize > size)
            return fail("Cinepak encoded frame size is invalid");
        if ((frameWidth && frameWidth != width_) ||
            (frameHeight && frameHeight != height_))
            return fail("Cinepak frame dimensions changed");
        if (stripCount > strips_.size())
            return fail("Cinepak frame has more than 32 strips");
        if (!stripCount)
            return true;

        if (segaSkipBytes_ < 0) {
            if (encodedSize != size && size % encodedSize != 0) {
                if (size >= 16 && data[10] == 0xFE && data[11] == 0x00 &&
                    data[12] == 0x00 && data[13] == 0x06 &&
                    data[14] == 0x00 && data[15] == 0x00)
                    segaSkipBytes_ = 6;
                else
                    segaSkipBytes_ = 2;
            } else {
                segaSkipBytes_ = 0;
            }
        }

        const std::size_t frameHeader = 10u + static_cast<std::size_t>(segaSkipBytes_);
        if (frameHeader + static_cast<std::size_t>(stripCount) * 12u > size)
            return fail("Cinepak strip headers are truncated");

        const std::uint8_t frameFlags = data[0];
        const std::uint8_t* cursor = data + frameHeader;
        const std::uint8_t* end = data + size;
        std::uint16_t previousBottom = 0;

        for (std::uint16_t stripIndex = 0; stripIndex < stripCount; ++stripIndex) {
            if (static_cast<std::size_t>(end - cursor) < 12)
                return fail("Cinepak strip header is truncated");

            Strip& strip = strips_[stripIndex];
            const std::uint32_t stripSize = read_be24(cursor + 1);
            if (stripSize < 12 || stripSize > static_cast<std::uint32_t>(end - cursor))
                return fail("Cinepak strip size is invalid");

            const std::uint16_t encodedY1 = read_be16(cursor + 4);
            strip.x1 = read_be16(cursor + 6);
            if (encodedY1 == 0) {
                strip.y1 = previousBottom;
                strip.y2 = static_cast<std::uint16_t>(
                    static_cast<std::uint32_t>(previousBottom) + read_be16(cursor + 8));
            } else {
                strip.y1 = encodedY1;
                strip.y2 = read_be16(cursor + 8);
            }
            strip.x2 = read_be16(cursor + 10);

            if (strip.x2 > width_ || strip.y2 > height_ ||
                strip.x1 >= strip.x2 || strip.y1 >= strip.y2)
                return fail("Cinepak strip coordinates are invalid");

            if (stripIndex > 0 && !(frameFlags & 0x01u)) {
                strip.v4 = strips_[stripIndex - 1].v4;
                strip.v1 = strips_[stripIndex - 1].v1;
            }

            if (!decode_strip(
                    strip,
                    cursor + 12,
                    static_cast<std::size_t>(stripSize - 12)))
                return false;

            cursor += stripSize;
            previousBottom = strip.y2;
        }
        return true;
    }

    VideoFrame frame() const override
    {
        return {
            pixels_.empty() ? nullptr : pixels_.data(),
            width_,
            height_,
            width_,
        };
    }

    bool present() override
    {
        const VideoFrame decoded = frame();
        if (!decoded.rgba ||
            !platform::renderer::movie_present_frame(
                decoded.rgba,
                decoded.width,
                decoded.height,
                decoded.pitchPixels))
            return fail("Neptune could not upload the Cinepak frame");
        return true;
    }

    const std::string& last_error() const override { return error_; }

private:
    bool fail(const char* message)
    {
        error_ = message ? message : "unknown Cinepak error";
        return false;
    }

    void put_pixel(std::uint32_t x, std::uint32_t y, std::uint32_t rgba)
    {
        if (x < width_ && y < height_)
            pixels_[static_cast<std::size_t>(y) * width_ + x] = rgba;
    }

    void draw_v1(const CodebookEntry& entry, std::uint32_t x, std::uint32_t y)
    {
        for (std::uint32_t row = 0; row < 4; ++row) {
            const std::uint32_t pair = row >= 2 ? 2u : 0u;
            put_pixel(x + 0, y + row, entry.rgba[pair + 0]);
            put_pixel(x + 1, y + row, entry.rgba[pair + 0]);
            put_pixel(x + 2, y + row, entry.rgba[pair + 1]);
            put_pixel(x + 3, y + row, entry.rgba[pair + 1]);
        }
    }

    void draw_v4_entry(
        const CodebookEntry& entry,
        std::uint32_t x,
        std::uint32_t y)
    {
        put_pixel(x + 0, y + 0, entry.rgba[0]);
        put_pixel(x + 1, y + 0, entry.rgba[1]);
        put_pixel(x + 0, y + 1, entry.rgba[2]);
        put_pixel(x + 1, y + 1, entry.rgba[3]);
    }

    bool decode_codebook(
        std::array<CodebookEntry, 256>& codebook,
        std::uint8_t chunkId,
        const std::uint8_t* data,
        std::size_t size)
    {
        const std::uint8_t* cursor = data;
        const std::uint8_t* end = data + size;
        const std::size_t encodedEntryBytes = (chunkId & 0x04u) ? 4u : 6u;
        std::uint32_t flags = 0;
        std::uint32_t mask = 0;

        for (std::size_t index = 0; index < codebook.size(); ++index) {
            if ((chunkId & 0x01u) && !(mask >>= 1)) {
                if (static_cast<std::size_t>(end - cursor) < 4)
                    return true;
                flags = read_be32(cursor);
                cursor += 4;
                mask = 0x80000000u;
            }

            if ((chunkId & 0x01u) && !(flags & mask))
                continue;
            if (static_cast<std::size_t>(end - cursor) < encodedEntryBytes)
                return true;

            CodebookEntry& entry = codebook[index];
            if (encodedEntryBytes == 4) {
                for (std::size_t pixel = 0; pixel < 4; ++pixel)
                    entry.rgba[pixel] = rgba_from_gray(*cursor++);
            } else {
                const std::uint8_t y[4] = {
                    cursor[0], cursor[1], cursor[2], cursor[3]};
                const std::int8_t u = static_cast<std::int8_t>(cursor[4]);
                const std::int8_t v = static_cast<std::int8_t>(cursor[5]);
                cursor += 6;
                for (std::size_t pixel = 0; pixel < 4; ++pixel)
                    entry.rgba[pixel] = rgba_from_yuv(y[pixel], u, v);
            }
        }
        return true;
    }

    bool decode_vectors(
        const Strip& strip,
        std::uint8_t chunkId,
        const std::uint8_t* data,
        std::size_t size)
    {
        const std::uint8_t* cursor = data;
        const std::uint8_t* end = data + size;
        std::uint32_t flags = 0;
        std::uint32_t mask = 0;

        for (std::uint32_t y = strip.y1; y < strip.y2; y += 4) {
            for (std::uint32_t x = strip.x1; x < strip.x2; x += 4) {
                if ((chunkId & 0x01u) && !(mask >>= 1)) {
                    if (static_cast<std::size_t>(end - cursor) < 4)
                        return fail("Cinepak interframe mask is truncated");
                    flags = read_be32(cursor);
                    cursor += 4;
                    mask = 0x80000000u;
                }

                if ((chunkId & 0x01u) && !(flags & mask))
                    continue;

                if (!(chunkId & 0x02u) && !(mask >>= 1)) {
                    if (static_cast<std::size_t>(end - cursor) < 4)
                        return fail("Cinepak vector mask is truncated");
                    flags = read_be32(cursor);
                    cursor += 4;
                    mask = 0x80000000u;
                }

                if ((chunkId & 0x02u) || (~flags & mask)) {
                    if (cursor >= end)
                        return fail("Cinepak V1 vector is truncated");
                    draw_v1(strip.v1[*cursor++], x, y);
                } else {
                    if (static_cast<std::size_t>(end - cursor) < 4)
                        return fail("Cinepak V4 vectors are truncated");
                    draw_v4_entry(strip.v4[cursor[0]], x + 0, y + 0);
                    draw_v4_entry(strip.v4[cursor[1]], x + 2, y + 0);
                    draw_v4_entry(strip.v4[cursor[2]], x + 0, y + 2);
                    draw_v4_entry(strip.v4[cursor[3]], x + 2, y + 2);
                    cursor += 4;
                }
            }
        }
        return true;
    }

    bool decode_strip(Strip& strip, const std::uint8_t* data, std::size_t size)
    {
        const std::uint8_t* cursor = data;
        const std::uint8_t* end = data + size;

        while (static_cast<std::size_t>(end - cursor) >= 4) {
            const std::uint8_t chunkId = cursor[0];
            const std::uint32_t chunkBytes = read_be24(cursor + 1);
            if (chunkBytes < 4 || chunkBytes > static_cast<std::uint32_t>(end - cursor))
                return fail("Cinepak chunk size is invalid");

            const std::uint8_t* chunkData = cursor + 4;
            const std::size_t chunkDataBytes = chunkBytes - 4;
            switch (chunkId) {
            case 0x20:
            case 0x21:
            case 0x24:
            case 0x25:
                if (!decode_codebook(strip.v4, chunkId, chunkData, chunkDataBytes))
                    return false;
                break;
            case 0x22:
            case 0x23:
            case 0x26:
            case 0x27:
                if (!decode_codebook(strip.v1, chunkId, chunkData, chunkDataBytes))
                    return false;
                break;
            case 0x30:
            case 0x31:
            case 0x32:
                return decode_vectors(strip, chunkId, chunkData, chunkDataBytes);
            default:
                break;
            }
            cursor += chunkBytes;
        }

        return fail("Cinepak strip contains no vector chunk");
    }

    std::uint32_t width_ = 0;
    std::uint32_t height_ = 0;
    int segaSkipBytes_ = -1;
    std::array<Strip, 32> strips_{};
    std::vector<std::uint32_t> pixels_;
    std::string error_;
};

} // namespace

std::unique_ptr<CinepakDecoder> make_cpu_cinepak_decoder()
{
    return std::make_unique<CpuCinepakDecoder>();
}

} // namespace lagi::movie

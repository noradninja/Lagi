#include "lagi/movie/cinepak_decoder.h"
#include "lagi/platform.h"

#include <array>
#include <cstddef>
#include <cstdint>
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

std::uint32_t pack_rgba(
    std::uint8_t r,
    std::uint8_t g,
    std::uint8_t b,
    std::uint8_t a)
{
    return static_cast<std::uint32_t>(r) |
           (static_cast<std::uint32_t>(g) << 8) |
           (static_cast<std::uint32_t>(b) << 16) |
           (static_cast<std::uint32_t>(a) << 24);
}

struct CodebookEntry {
    std::uint8_t y[4]{};
    std::int8_t u = 0;
    std::int8_t v = 0;
};

struct Strip {
    std::uint16_t x1 = 0;
    std::uint16_t y1 = 0;
    std::uint16_t x2 = 0;
    std::uint16_t y2 = 0;
    std::array<CodebookEntry, 256> v4{};
    std::array<CodebookEntry, 256> v1{};
};

class SgxCinepakDecoder final : public CinepakDecoder {
public:
    bool configure(std::uint32_t width, std::uint32_t height) override
    {
        error_.clear();
        if (!width || !height || width > 4096 || height > 4096)
            return fail("Cinepak dimensions are invalid");

        width_ = width;
        height_ = height;
        blockColumns_ = (width + 3u) / 4u;
        blockRows_ = (height + 3u) / 4u;
        payloadWidth_ = blockColumns_ * 8u;
        payload_.assign(
            static_cast<std::size_t>(payloadWidth_) * blockRows_,
            0u);

        // Initialize every persistent block as neutral black. Interframe
        // skips then naturally preserve whatever the last decoded frame wrote.
        const CodebookEntry black{};
        for (std::uint32_t by = 0; by < blockRows_; ++by) {
            for (std::uint32_t bx = 0; bx < blockColumns_; ++bx)
                write_v1_block(bx, by, black);
        }

        strips_ = {};
        segaSkipBytes_ = -1;
        return true;
    }

    bool decode(const std::uint8_t* data, std::size_t size) override
    {
        error_.clear();
        if (!width_ || !height_ || payload_.empty())
            return fail("SGX Cinepak decoder is not configured");
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

        const std::size_t frameHeader =
            10u + static_cast<std::size_t>(segaSkipBytes_);
        if (frameHeader + static_cast<std::size_t>(stripCount) * 12u > size)
            return fail("Cinepak strip headers are truncated");

        const std::uint8_t frameFlags = data[0];
        const std::uint8_t* cursor = data + frameHeader;
        const std::uint8_t* end = data + size;
        std::uint16_t previousBottom = 0;

        for (std::uint16_t stripIndex = 0;
             stripIndex < stripCount;
             ++stripIndex) {
            if (static_cast<std::size_t>(end - cursor) < 12)
                return fail("Cinepak strip header is truncated");

            Strip& strip = strips_[stripIndex];
            const std::uint32_t stripSize = read_be24(cursor + 1);
            if (stripSize < 12 ||
                stripSize > static_cast<std::uint32_t>(end - cursor))
                return fail("Cinepak strip size is invalid");

            const std::uint16_t encodedY1 = read_be16(cursor + 4);
            strip.x1 = read_be16(cursor + 6);
            if (encodedY1 == 0) {
                strip.y1 = previousBottom;
                strip.y2 = static_cast<std::uint16_t>(
                    static_cast<std::uint32_t>(previousBottom) +
                    read_be16(cursor + 8));
            } else {
                strip.y1 = encodedY1;
                strip.y2 = read_be16(cursor + 8);
            }
            strip.x2 = read_be16(cursor + 10);

            if (strip.x2 > width_ || strip.y2 > height_ ||
                strip.x1 >= strip.x2 || strip.y1 >= strip.y2)
                return fail("Cinepak strip coordinates are invalid");

            if ((strip.x1 & 3u) || (strip.y1 & 3u))
                return fail("SGX Cinepak requires 4-pixel aligned strips");

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

    bool present() override
    {
        if (!platform::renderer::movie_present_cinepak_payload(
                payload_.data(),
                payloadWidth_,
                blockRows_,
                width_,
                height_))
            return fail("Neptune could not present the SGX Cinepak payload");
        return true;
    }

    VideoFrame frame() const override
    {
        // SGX reconstruction intentionally has no CPU RGBA frame.
        return {nullptr, width_, height_, 0};
    }

    const std::string& last_error() const override { return error_; }

private:
    bool fail(const char* message)
    {
        error_ = message ? message : "unknown SGX Cinepak error";
        return false;
    }

    void write_entry(
        std::uint32_t blockX,
        std::uint32_t blockY,
        std::uint32_t entryIndex,
        const CodebookEntry& entry,
        bool v4Mode)
    {
        if (blockX >= blockColumns_ || blockY >= blockRows_ ||
            entryIndex >= 4u)
            return;

        const std::size_t base =
            static_cast<std::size_t>(blockY) * payloadWidth_ +
            blockX * 8u + entryIndex * 2u;
        payload_[base + 0u] = pack_rgba(
            entry.y[0], entry.y[1], entry.y[2], entry.y[3]);
        payload_[base + 1u] = pack_rgba(
            static_cast<std::uint8_t>(
                static_cast<int>(entry.u) + 128),
            static_cast<std::uint8_t>(
                static_cast<int>(entry.v) + 128),
            (entryIndex == 0u && v4Mode) ? 255u : 0u,
            255u);
    }

    void write_v1_block(
        std::uint32_t blockX,
        std::uint32_t blockY,
        const CodebookEntry& entry)
    {
        write_entry(blockX, blockY, 0u, entry, false);
    }

    void write_v4_block(
        std::uint32_t blockX,
        std::uint32_t blockY,
        const CodebookEntry& e0,
        const CodebookEntry& e1,
        const CodebookEntry& e2,
        const CodebookEntry& e3)
    {
        write_entry(blockX, blockY, 0u, e0, true);
        write_entry(blockX, blockY, 1u, e1, true);
        write_entry(blockX, blockY, 2u, e2, true);
        write_entry(blockX, blockY, 3u, e3, true);
    }

    bool decode_codebook(
        std::array<CodebookEntry, 256>& codebook,
        std::uint8_t chunkId,
        const std::uint8_t* data,
        std::size_t size)
    {
        const std::uint8_t* cursor = data;
        const std::uint8_t* end = data + size;
        const std::size_t encodedEntryBytes =
            (chunkId & 0x04u) ? 4u : 6u;
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
            entry.y[0] = cursor[0];
            entry.y[1] = cursor[1];
            entry.y[2] = cursor[2];
            entry.y[3] = cursor[3];
            if (encodedEntryBytes == 4) {
                entry.u = 0;
                entry.v = 0;
                cursor += 4;
            } else {
                entry.u = static_cast<std::int8_t>(cursor[4]);
                entry.v = static_cast<std::int8_t>(cursor[5]);
                cursor += 6;
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

                const std::uint32_t blockX = x >> 2;
                const std::uint32_t blockY = y >> 2;
                if ((chunkId & 0x02u) || (~flags & mask)) {
                    if (cursor >= end)
                        return fail("Cinepak V1 vector is truncated");
                    write_v1_block(
                        blockX, blockY, strip.v1[*cursor++]);
                } else {
                    if (static_cast<std::size_t>(end - cursor) < 4)
                        return fail("Cinepak V4 vectors are truncated");
                    write_v4_block(
                        blockX, blockY,
                        strip.v4[cursor[0]],
                        strip.v4[cursor[1]],
                        strip.v4[cursor[2]],
                        strip.v4[cursor[3]]);
                    cursor += 4;
                }
            }
        }
        return true;
    }

    bool decode_strip(
        Strip& strip,
        const std::uint8_t* data,
        std::size_t size)
    {
        const std::uint8_t* cursor = data;
        const std::uint8_t* end = data + size;

        while (static_cast<std::size_t>(end - cursor) >= 4) {
            const std::uint8_t chunkId = cursor[0];
            const std::uint32_t chunkBytes = read_be24(cursor + 1);
            if (chunkBytes < 4 ||
                chunkBytes > static_cast<std::uint32_t>(end - cursor))
                return fail("Cinepak chunk size is invalid");

            const std::uint8_t* chunkData = cursor + 4;
            const std::size_t chunkDataBytes = chunkBytes - 4;
            switch (chunkId) {
            case 0x20:
            case 0x21:
            case 0x24:
            case 0x25:
                if (!decode_codebook(
                        strip.v4, chunkId, chunkData, chunkDataBytes))
                    return false;
                break;
            case 0x22:
            case 0x23:
            case 0x26:
            case 0x27:
                if (!decode_codebook(
                        strip.v1, chunkId, chunkData, chunkDataBytes))
                    return false;
                break;
            case 0x30:
            case 0x31:
            case 0x32:
                return decode_vectors(
                    strip, chunkId, chunkData, chunkDataBytes);
            default:
                break;
            }
            cursor += chunkBytes;
        }

        return fail("Cinepak strip contains no vector chunk");
    }

    std::uint32_t width_ = 0;
    std::uint32_t height_ = 0;
    std::uint32_t blockColumns_ = 0;
    std::uint32_t blockRows_ = 0;
    std::uint32_t payloadWidth_ = 0;
    int segaSkipBytes_ = -1;
    std::array<Strip, 32> strips_{};
    std::vector<std::uint32_t> payload_;
    std::string error_;
};

} // namespace

std::unique_ptr<CinepakDecoder> make_sgx_cinepak_decoder()
{
    return std::make_unique<SgxCinepakDecoder>();
}

} // namespace lagi::movie

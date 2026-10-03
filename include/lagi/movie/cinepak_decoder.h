#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

namespace lagi::movie {

struct VideoFrame {
    const std::uint32_t* rgba = nullptr;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::uint32_t pitchPixels = 0;
};

// Phase 2 implements this same decode/present contract with SGX codebook and
// block-map resources. FILM demux, clocks, audio, and Azel ownership stay
// intact; only this video backend changes.
class CinepakDecoder {
public:
    virtual ~CinepakDecoder() = default;
    virtual bool configure(std::uint32_t width, std::uint32_t height) = 0;
    virtual bool decode(const std::uint8_t* data, std::size_t size) = 0;
    virtual bool present() = 0;
    virtual VideoFrame frame() const = 0;
    virtual const std::string& last_error() const = 0;
};

std::unique_ptr<CinepakDecoder> make_cpu_cinepak_decoder();

} // namespace lagi::movie

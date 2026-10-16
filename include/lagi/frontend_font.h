#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

namespace lagi::frontend {
struct UnpackResult {bool ok=false;std::size_t written=0,consumed=0;};
// Bounded form of Azel VDP2.cpp::unpackGraphicsToVDP2, including overlapping
// negative-distance copies and the native zero-word end marker.
UnpackResult unpackFont(const std::uint8_t* source,std::size_t bytes,
    std::uint8_t* destination,std::size_t capacity);

class LatinFont {
public:
    static constexpr int width=8,height=16,advance=8;
    static constexpr std::size_t maximumCompressedBytes=65536;
    static constexpr std::size_t maximumDecodedBytes=32768;
    bool load(const std::uint8_t* compressed,std::size_t bytes,
        const std::uint8_t* nativePalette32);
    bool ready()const{return ready_;}
    std::size_t decodedBytes()const{return decodedBytes_;}
    int measure(const char* text)const;
    std::uint8_t pixel(unsigned character,unsigned x,unsigned y)const;
    void draw(std::uint32_t* pixels,int width,int height,int pitch,int x,int y,
        const char* text,std::uint32_t tint=0xFFFFFFFFu)const;
private:
    std::array<std::uint8_t,95*8*16> glyphs_{};
    std::array<std::uint32_t,16> palette_{};
    std::size_t decodedBytes_=0;
    bool ready_=false;
};
// Vita frontend cache: initialized before the first Loading frame. Reads game
// resources without creating Azel tasks or changing its native text state.
bool initializeFont();
const LatinFont& font();
}

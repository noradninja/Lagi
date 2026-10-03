#include <chrono>
#pragma once
#include "lagi/lagi_compat.h"

namespace ImGui {
struct IO { bool MouseClicked[5]{}; };
inline IO& GetIO() { static IO io{}; return io; }
inline bool InputFloat(const char*, float*, float = 0.0f, float = 0.0f) { return false; }
inline bool Begin(const char*, bool* = nullptr) { return false; }
inline void End() {}
inline bool Checkbox(const char*, bool*) { return false; }
inline void PushItemWidth(float) {}
inline void SameLine() {}
inline void PopItemWidth() {}
inline void Text(const char*, ...) {}
inline void PushID(const char*) {}
inline void PopID() {}
}

namespace glm {
template<typename T> constexpr T pi() { return static_cast<T>(3.14159265358979323846); }
template<typename T> constexpr T degrees(T v) { return v * static_cast<T>(57.2957795130823208768); }
template<typename T> constexpr T radians(T v) { return v * static_cast<T>(0.01745329251994329577); }
}


namespace SoLoud {
using handle = unsigned int;
static constexpr int SO_NO_ERROR = 0;

class Wav {
public:
    int loadRawWave16(const short*, unsigned int, float, unsigned int) { return SO_NO_ERROR; }
};

class WavStream {
public:
    int load(const char*) { return SO_NO_ERROR; }
    void setLooping(bool) {}
};

class Soloud {
public:
    int init() { return SO_NO_ERROR; }
    handle play(Wav&) { return 1; }
    handle play(WavStream&) { return 1; }
    bool isValidVoiceHandle(handle h) const { return h != 0; }
    void stop(handle) {}
};
}

inline SoLoud::Soloud gSoloud;

inline std::uint64_t SDL_GetPerformanceCounter()
{
    using namespace std::chrono;
    return static_cast<std::uint64_t>(
        duration_cast<microseconds>(steady_clock::now().time_since_epoch()).count());
}

inline std::uint64_t SDL_GetPerformanceFrequency()
{
    return 1000000ull;
}

namespace bgfx {
enum class Attrib { Position, TexCoord0, TexCoord1 };
enum class AttribType { Float };
enum class TextureFormat { RGBA8, RGBA32F };

struct Memory {};

struct VertexLayout {
    VertexLayout& begin() { return *this; }
    VertexLayout& add(Attrib, std::uint8_t, AttribType) { return *this; }
    void end() {}
};

struct VertexBufferHandle { std::uint16_t idx = 0xFFFFu; };
struct IndexBufferHandle { std::uint16_t idx = 0xFFFFu; };
struct TextureHandle { std::uint16_t idx = 0xFFFFu; };

inline bool isValid(VertexBufferHandle h) { return h.idx != 0xFFFFu; }
inline bool isValid(IndexBufferHandle h) { return h.idx != 0xFFFFu; }
inline bool isValid(TextureHandle h) { return h.idx != 0xFFFFu; }

inline void destroy(VertexBufferHandle) {}
inline void destroy(IndexBufferHandle) {}
inline void destroy(TextureHandle) {}

inline const Memory* copy(const void*, std::uint32_t) {
    static Memory memory{};
    return &memory;
}

inline TextureHandle createTexture2D(
    std::uint16_t, std::uint16_t, bool, std::uint16_t,
    TextureFormat, std::uint64_t, const Memory*) { return {}; }

inline TextureHandle createTexture2D(
    std::uint16_t, std::uint16_t, bool, std::uint16_t,
    TextureFormat, const Memory*) { return {}; }

inline VertexBufferHandle createVertexBuffer(const Memory*, const VertexLayout&) { return {}; }
inline IndexBufferHandle createIndexBuffer(const Memory*) { return {}; }
}

#ifndef BGFX_INVALID_HANDLE
#define BGFX_INVALID_HANDLE { 0xFFFFu }
#endif


#ifndef BGFX_SAMPLER_POINT
#define BGFX_SAMPLER_POINT   (UINT64_C(1) << 0)
#endif
#ifndef BGFX_SAMPLER_U_CLAMP
#define BGFX_SAMPLER_U_CLAMP (UINT64_C(1) << 1)
#endif
#ifndef BGFX_SAMPLER_V_CLAMP
#define BGFX_SAMPLER_V_CLAMP (UINT64_C(1) << 2)
#endif

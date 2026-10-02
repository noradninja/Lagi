#pragma once
#include "lagi/lagi_compat.h"

namespace ImGui {
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

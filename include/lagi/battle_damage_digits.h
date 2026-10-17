#pragma once
#include <cstdint>
void lagiDrawBattleDamageDigits(const char* digits, std::int16_t x, std::int16_t y,
    std::int16_t width, std::int16_t height, std::uint16_t vdp1Base,
    std::uint16_t colorType, std::int32_t mode);

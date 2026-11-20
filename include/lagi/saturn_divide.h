#pragma once
#include <cstdint>
#include <limits>
namespace lagi {
// MAIN 06035686 uses DMULS.L then the SH7604 signed 64/32 DIVU.
// Return the quotient register, including its saturated overflow result;
// no CPU assertion or divide exception replaces the peripheral operation.
inline std::int32_t saturn_mul_div(std::int32_t a,std::int32_t b,std::int32_t divisor)
{
    const std::int64_t dividend=std::int64_t(a)*b;
    constexpr auto low=std::numeric_limits<std::int32_t>::min();
    constexpr auto high=std::numeric_limits<std::int32_t>::max();
    if(!divisor)return dividend<0?low:high;
    const std::int64_t quotient=dividend/divisor;
    return quotient<low?low:quotient>high?high:static_cast<std::int32_t>(quotient);
}
}

// Vita-native build of Azel's portable fixed-point implementation.
// Math behavior mirrors upstream AzelLib/fixedPoint.cpp; desktop-only ImGui/GLM
// editor helpers are intentionally excluded.

#include "lagi/azel_compat.h"
#include "heap.h"
#include "task.h"
#include "VDP1.h"
#include "VDP2.h"
#include "common.h"

s32 FP_GetIntegerPortion(fixedPoint& FP)
{
    return FP.asS32() >> 16;
}

fixedPoint FP_Div(s32 dividend, fixedPoint divisor)
{
    if (divisor == 0)
        return dividend >= 0 ? fixedPoint(0x7FFFFFFF) : fixedPoint(static_cast<s32>(0x80000000u));

    return fixedPoint::fromS32((static_cast<s64>(dividend) * 0x10000) / divisor.asS32());
}

sVec3_FP FP_Div(sVec3_FP dividend, fixedPoint divisor)
{
    sVec3_FP result;
    result[0] = FP_Div(dividend[0], divisor);
    result[1] = FP_Div(dividend[1], divisor);
    result[2] = FP_Div(dividend[2], divisor);
    return result;
}

fixedPoint dot3_FP(const sVec3_FP* a, const sVec3_FP* b)
{
    s64 acc = 0;
    acc += (*a)[0] * static_cast<s64>((*b)[0]);
    acc += (*a)[1] * static_cast<s64>((*b)[1]);
    acc += (*a)[2] * static_cast<s64>((*b)[2]);
    return static_cast<s32>(acc >> 16);
}

fixedPoint MTH_Product3d_FP(const sVec3_FP& a, const sVec3_FP& b)
{
    return dot3_FP(&a, &b);
}

s64 gDivident;
s64 gDivisor;

void asyncDivStart(s32 value, fixedPoint divisor)
{
    gDivident = static_cast<s64>(value) << 16;
    gDivisor = divisor;
}

void asyncDivStart_integer(s32 value, s32 divisor)
{
    gDivident = value;
    gDivisor = divisor;
}

fixedPoint asyncDivEnd()
{
    if (gDivisor == 0)
        return gDivident >= 0 ? fixedPoint(0x7FFFFFFF) : fixedPoint(static_cast<s32>(0x80000000u));
    return fixedPoint::fromS32(gDivident / gDivisor);
}

fixedPoint intDivide(fixedPoint divisor, fixedPoint dividend)
{
    return dividend / divisor;
}

s64 MUL_FP(const fixedPoint& a, const fixedPoint& b)
{
    return static_cast<s64>(a.asS32()) * static_cast<s64>(b.asS32());
}

fixedPoint MTH_Mul_5_6(fixedPoint a, fixedPoint b, fixedPoint c)
{
    return MTH_Mul(a, MTH_Mul(b, c));
}

fixedPoint MTH_Mul(fixedPoint a, fixedPoint b)
{
    return fixedPoint(static_cast<s32>((static_cast<s64>(a.asS32()) * b.asS32()) >> 16));
}

sVec3_FP MTH_Mul(const fixedPoint& a, const sVec3_FP& b)
{
    sVec3_FP result;
    result[0] = MTH_Mul(a, b[0]);
    result[1] = MTH_Mul(a, b[1]);
    result[2] = MTH_Mul(a, b[2]);
    return result;
}

sVec3_FP MTH_Mul(const sVec3_FP& a, const sVec3_FP& b)
{
    sVec3_FP result;
    result[0] = MTH_Mul(a[0], b[0]);
    result[1] = MTH_Mul(a[1], b[1]);
    result[2] = MTH_Mul(a[2], b[2]);
    return result;
}

sVec3_FP MTH_Mul(const sVec3_FP& a, const fixedPoint& b)
{
    return MTH_Mul(b, a);
}

fixedPoint FP_Pow2(fixedPoint value)
{
    const s64 result = static_cast<s64>(value.asS32()) * value.asS32();
    return fixedPoint::fromS32(static_cast<s32>(result >> 16));
}

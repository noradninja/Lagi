#pragma once
#include <cstdint>
struct sVec3_FP;
// Shared native untextured world quad (FLD_A7 06069AD0 / 0606993C).
// A negative sort depth selects the first near-clipped view-space vertex Z.
void lagiDisplayWorldPolygon(const sVec3_FP* points,std::uint16_t color,std::int32_t depth=-1);

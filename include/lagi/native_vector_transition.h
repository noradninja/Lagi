#pragma once
#include <cstdint>
struct s_workArea;
struct sVec3_FP;
// BTL_A5_3 060540E2/06054078. The parent owns the live destination vector.
// The authored endpoint is sampled at creation; movement never snaps to it.
s_workArea* lagiCreateNativeVectorTransition(s_workArea* parent,sVec3_FP* live,
    const sVec3_FP& endpoint,std::int16_t frames);

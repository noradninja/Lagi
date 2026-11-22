#pragma once
#include <cstdint>
struct s_workArea;
struct sSaturnPtr;
// Original negative-flag speech factory (A5_3 06068B66). Copies parameters;
// no borrowed entry table survives creation and no next entry is read.
s_workArea* lagiCreateNativeBattleSpeech(s_workArea* parent,std::int16_t text,
    std::int16_t duration,sSaturnPtr filename);

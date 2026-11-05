#pragma once
#include <cstdint>

struct s_workArea;
struct s_multiChoiceTask;

void lagiStartTownYesNoDialog(s_workArea* parent, s_multiChoiceTask** output,
                              std::int32_t* result);

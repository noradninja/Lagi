#pragma once
#include <cstdint>
struct s_workArea;
namespace lagi {
// Required table end verified by the read-only US Disc 1 constructor test.
// This is a missing/truncated-resource guard, not a malformed-game-data parser.
constexpr std::uint32_t battle_generic_minimum_bytes=0x060B08F8-0x06054000;
}
bool lagiPrepareNativeA3Battle();
bool lagiEnterNativeA3Battle(s_workArea* parent);

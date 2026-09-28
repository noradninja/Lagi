#pragma once
#include <cstdint>
#include <vector>

namespace lagi::disc {
bool init();
bool mounted();
const char* image_path();
bool has_file(const char* name);
bool read_file(const char* name, std::vector<std::uint8_t>& out);
}

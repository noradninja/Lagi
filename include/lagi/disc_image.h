#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

namespace lagi::disc {
struct FileHandle {
    void* native = nullptr;
    std::uint32_t extent = 0;
    std::uint32_t size = 0;
};

bool init();
bool mounted();
const char* image_path();
bool has_file(const char* name);
bool open_file(const char* name, FileHandle& handle);
void close_file(FileHandle& handle);
bool read_file_at(
    FileHandle& handle,
    std::uint64_t offset,
    void* destination,
    std::size_t bytes);
bool read_file(const char* name, std::vector<std::uint8_t>& out);
}

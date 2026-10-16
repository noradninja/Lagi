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
// Disc selection is a pre-Azel platform operation; valid numbers are 1-4.
bool mount_disc(int discNumber);
bool disc_available(int discNumber);
int selected_disc();
bool mounted();
const char* image_path();
bool has_file(const char* name);
bool file_size(const char* name, std::uint32_t& out);
bool open_file(const char* name, FileHandle& handle);
void close_file(FileHandle& handle);
bool read_file_at(
    FileHandle& handle,
    std::uint64_t offset,
    void* destination,
    std::size_t bytes);
bool read_file(const char* name, std::vector<std::uint8_t>& out);
// Frontend-only bounded probe. Restore every mount/selection field on return.
bool read_file_from_disc(int discNumber,const char* name,
    std::vector<std::uint8_t>& out,std::size_t maximumBytes);
}

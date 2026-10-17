#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include <initializer_list>

namespace lagi::resident_video {
enum class Space { vdp1, vdp2, cram };
struct Span {
    std::string filename;
    Space space;
    std::size_t destination, source, length, fileSize;
};
struct PreparedFile { std::string filename; std::vector<std::uint8_t> bytes; };
struct Snapshot {
    int disc=-1;
    std::vector<Span> spans;
    std::vector<PreparedFile> files;
    bool prepared=false;
    std::string error;
};
// Only permanent native video-memory owners are tracked. Heap bundles remain
// owned by Azel and must not be reallocated or re-patched during field return.
void record_load(const char* filename, std::uint8_t* destination, std::size_t size);
void capture(Snapshot& snapshot);
// Read/validate everything before any destination is changed. Failure can be
// retried while the field remains paused, without replaying audio cleanup.
bool prepare(Snapshot& snapshot);
bool restore(Snapshot& snapshot);
struct Range { Space space; std::size_t destination,length; };
// Mirror existing menu byte backups, without copying any extra game data.
void save_scope(const void* owner,std::initializer_list<Range> ranges);
void restore_scope(const void* owner);
}

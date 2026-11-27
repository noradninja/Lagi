#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace neptune {

// Metadata only. The backend owns the mapped memory. The allocator never
// writes memory, submits GPU commands, waits, or guesses GPU completion.
template<std::size_t MaxRanges = 1024>
class GeometryArena {
public:
    struct Range {
        std::uint64_t id = 0;
        std::size_t offset = 0, bytes = 0;
        explicit operator bool() const noexcept { return id != 0; }
    };
    explicit GeometryArena(std::size_t capacity = 0) noexcept
        : capacity_(capacity) {}
    GeometryArena(const GeometryArena&) = delete;
    GeometryArena& operator=(const GeometryArena&) = delete;

    // Offsets are aligned relative to a suitably aligned backend base.
    Range allocate(std::size_t bytes, std::size_t alignment) noexcept {
        if (!bytes || !alignment || (alignment & (alignment-1)) ||
            count_ == MaxRanges || next_ == std::numeric_limits<std::uint64_t>::max())
            return {};
        std::size_t cursor = 0;
        for (std::size_t i = 0; i <= count_; ++i) {
            const auto end = i == count_ ? capacity_ : entries_[i].range.offset;
            if (cursor <= end && cursor <= std::numeric_limits<std::size_t>::max()-(alignment-1)) {
                const auto offset = (cursor+alignment-1)&~(alignment-1);
                if (offset <= end && bytes <= end-offset) {
                    for (auto j=count_; j>i; --j) entries_[j]=entries_[j-1];
                    Range range{++next_,offset,bytes};
                    entries_[i] = {range,0,false};
                    ++count_; used_+=bytes;
                    if (used_>peak_) peak_=used_;
                    return range;
                }
            }
            if (i<count_) cursor=entries_[i].range.offset+entries_[i].range.bytes;
        }
        return {};
    }

    // Serial must refer to actual submitted work. The caller may advance
    // completion ONLY after the corresponding GPU fence/finish succeeds.
    bool markUsed(Range range, std::uint64_t serial) noexcept {
        auto* e=find(range);
        if (!e || e->retired || !serial || serial<=completed_) return false;
        if (serial>e->lastUse) e->lastUse=serial;
        return true;
    }
    bool retire(Range range) noexcept {
        auto* e=find(range);
        if (!e || e->retired) return false;
        e->retired=true;
        reclaim();
        return true;
    }
    bool completeThrough(std::uint64_t serial) noexcept {
        if (serial<completed_) return false;
        completed_=serial;
        reclaim();
        return true;
    }
    std::size_t capacity() const noexcept { return capacity_; }
    std::size_t used() const noexcept { return used_; }
    std::size_t peak() const noexcept { return peak_; }
    std::size_t count() const noexcept { return count_; }
    std::size_t largestFree() const noexcept {
        std::size_t cursor=0, largest=0;
        for (std::size_t i=0;i<count_;++i) {
            const auto& r=entries_[i].range;
            const auto gap=r.offset-cursor;
            if (gap>largest) largest=gap;
            cursor=r.offset+r.bytes;
        }
        const auto tail=capacity_-cursor;
        return tail>largest?tail:largest;
    }

private:
    struct Entry { Range range{}; std::uint64_t lastUse=0; bool retired=false; };
    Entry* find(Range range) noexcept {
        if (!range) return nullptr;
        for (std::size_t i=0;i<count_;++i) {
            auto& e=entries_[i];
            if (e.range.id==range.id && e.range.offset==range.offset &&
                e.range.bytes==range.bytes) return &e;
        }
        return nullptr;
    }
    void reclaim() noexcept {
        std::size_t out=0;
        for (std::size_t i=0;i<count_;++i) {
            auto& e=entries_[i];
            if (e.retired && e.lastUse<=completed_) used_-=e.range.bytes;
            else entries_[out++]=e;
        }
        count_=out;
    }
    std::array<Entry,MaxRanges> entries_{};
    const std::size_t capacity_;
    std::size_t count_=0, used_=0, peak_=0;
    std::uint64_t next_=0, completed_=0;
};

} // namespace neptune

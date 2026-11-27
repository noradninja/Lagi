#pragma once
#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <unordered_map>

namespace neptune {

// Metadata only. Single owner thread. Handles are arena-local; never carry
// them across backing block lifetimes. Offsets assume aligned backing base.
template<std::size_t MaxRanges = 1024>
class GeometryArena {
public:
    struct Range {
        std::uint64_t id = 0;
        std::size_t offset = 0, bytes = 0;
        explicit operator bool() const noexcept { return id != 0; }
    };
    explicit GeometryArena(std::size_t capacity = 0)
        : capacity_(capacity) {
        if (capacity) free_.emplace(0,capacity);
    }
    GeometryArena(const GeometryArena&) = delete;
    GeometryArena& operator=(const GeometryArena&) = delete;

    // First fit over free extents, rather than over live allocations.
    // Split only the chosen extent. No shifting live records.
    Range allocate(std::size_t bytes, std::size_t alignment) {
        if (!bytes || !alignment || (alignment & (alignment-1)) ||
            entries_.size() >= MaxRanges ||
            next_ == std::numeric_limits<std::uint64_t>::max())
            return {};
        for (auto it=free_.begin();it!=free_.end();++it) {
            const auto start=it->first, length=it->second;
            if (start>std::numeric_limits<std::size_t>::max()-(alignment-1))
                continue;
            const auto offset=(start+alignment-1)&~(alignment-1);
            const auto padding=offset-start;
            if (padding>length || bytes>length-padding) continue;
            const auto tail=length-padding-bytes;
            Range range{next_+1,offset,bytes};
            entries_.emplace(range.id,Entry{range,0,false});
            free_.erase(it);
            if (padding) free_.emplace(start,padding);
            if (tail) free_.emplace(offset+bytes,tail);
            ++next_;
            used_+=bytes;
            if (used_>peak_) peak_=used_;
            return range;
        }
        return {};
    }

    bool markUsed(Range range, std::uint64_t serial) noexcept {
        auto it=find(range);
        if (it==entries_.end() || it->second.retired ||
            !serial || serial<=completed_) return false;
        if (serial>it->second.lastUse) it->second.lastUse=serial;
        return true;
    }
    bool retire(Range range) {
        auto it=find(range);
        if (it==entries_.end() || it->second.retired) return false;
        if (it->second.lastUse<=completed_) release(it);
        else {
            pending_.emplace(it->second.lastUse,range.id);
            it->second.retired=true;
        }
        return true;
    }
    // Only a proven completion boundary may advance this serial.
    bool completeThrough(std::uint64_t serial) {
        if (serial<completed_) return false;
        completed_=serial;
        while (!pending_.empty() && pending_.begin()->first<=serial) {
            auto it=entries_.find(pending_.begin()->second);
            if (it!=entries_.end()) release(it);
            pending_.erase(pending_.begin());
        }
        return true;
    }
    std::size_t capacity() const noexcept { return capacity_; }
    std::size_t used() const noexcept { return used_; }
    std::size_t peak() const noexcept { return peak_; }
    std::size_t count() const noexcept { return entries_.size(); }
    std::size_t largestFree() const noexcept {
        std::size_t largest=0;
        for (const auto& extent:free_)
            if (extent.second>largest) largest=extent.second;
        return largest;
    }

private:
    struct Entry { Range range{}; std::uint64_t lastUse=0; bool retired=false; };
    using Entries=std::unordered_map<std::uint64_t,Entry>;
    typename Entries::iterator find(Range range) noexcept {
        auto it=entries_.find(range.id);
        if (!range || it==entries_.end() ||
            it->second.range.offset!=range.offset ||
            it->second.range.bytes!=range.bytes) return entries_.end();
        return it;
    }
    void release(typename Entries::iterator it) {
        auto start=it->second.range.offset, bytes=it->second.range.bytes;
        auto right=free_.lower_bound(start);
        if (right!=free_.begin()) {
            auto left=right; --left;
            if (left->first+left->second==start) {
                start=left->first;
                bytes+=left->second;
                free_.erase(left);
            }
        }
        if (right!=free_.end() && start+bytes==right->first) {
            bytes+=right->second;
            free_.erase(right);
        }
        free_.emplace(start,bytes);
        used_-=it->second.range.bytes;
        entries_.erase(it);
    }
    Entries entries_;
    std::map<std::size_t,std::size_t> free_;
    std::multimap<std::uint64_t,std::uint64_t> pending_;
    const std::size_t capacity_;
    std::size_t used_=0, peak_=0;
    std::uint64_t next_=0, completed_=0;
};

} // namespace neptune

#pragma once
#include "neptune/geometry_resource_key.h"
#include <limits>
#include <unordered_map>

namespace neptune {

// CPU-side, single-owner-thread registry. The caller supplies a nonzero
// producer namespace unique across registry lifetimes. Owner tokens are
// opaque adapter lifetime IDs, not addresses. Retire before owner reuse.
// GPU allocation/retirement is intentionally separate.
class GeometryResourceRegistry {
public:
    explicit GeometryResourceRegistry(std::uint64_t producer) noexcept
        : producer_(producer) {}
    GeometryResourceRegistry(const GeometryResourceRegistry&) = delete;
    GeometryResourceRegistry& operator=(const GeometryResourceRegistry&) = delete;

    GeometryResourceKey create(std::uint64_t owner) {
        if (!producer_ || !owner || next_ == maxId) return {};
        const auto id = next_ + 1;
        entries_.emplace(id, Entry{owner, 1});
        next_ = id;
        return {producer_, id, 1};
    }
    bool current(const GeometryResourceKey& key) const noexcept {
        if (!key.valid() || key.producer != producer_) return false;
        const auto it = entries_.find(key.resource);
        return it != entries_.end() && it->second.revision == key.revision;
    }
    GeometryResourceKey revise(const GeometryResourceKey& key) noexcept {
        if (!current(key)) return {};
        auto it = entries_.find(key.resource);
        if (it->second.revision == maxId) {
            entries_.erase(it); // Fail closed, never wrap.
            return {};
        }
        return {producer_, key.resource, ++it->second.revision};
    }
    bool retire(const GeometryResourceKey& key) noexcept {
        if (!current(key)) return false;
        entries_.erase(key.resource);
        return true;
    }
    std::size_t retireOwner(std::uint64_t owner) noexcept {
        if (!owner) return 0;
        std::size_t count = 0;
        for (auto it = entries_.begin(); it != entries_.end();) {
            if (it->second.owner == owner) {
                it = entries_.erase(it);
                ++count;
            } else ++it;
        }
        return count;
    }
    std::size_t size() const noexcept { return entries_.size(); }

private:
    struct Entry { std::uint64_t owner, revision; };
    static constexpr auto maxId = std::numeric_limits<std::uint64_t>::max();
    const std::uint64_t producer_;
    std::uint64_t next_ = 0;
    std::unordered_map<std::uint64_t, Entry> entries_;
};

} // namespace neptune

#pragma once

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <utility>

namespace lagi {

// Producer-thread cache. Published frames retain immutable snapshots, not the
// native resource. Retirement permits address reuse without invalidating a
// frame that has already captured the old geometry.
template<class Key, class Value>
class ModelSnapshotCache {
public:
    struct Snapshot {
        std::uint64_t generation;
        Value value;
    };
    using Handle = std::shared_ptr<const Snapshot>;

    Handle find(Key key) const {
        const auto it = entries_.find(key);
        return it == entries_.end() ? Handle{} : it->second;
    }
    Handle insert(Key key, Value value) {
        if (auto existing = find(key)) return existing;
        auto snapshot = std::make_shared<const Snapshot>(
            Snapshot{++generation_, std::move(value)});
        entries_.emplace(key, snapshot);
        return snapshot;
    }
    void retire(Key key) { entries_.erase(key); }

private:
    std::uint64_t generation_ = 0;
    std::unordered_map<Key, Handle> entries_;
};

} // namespace lagi

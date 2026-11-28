#pragma once
#include "neptune/geometry_arena.h"
#include "neptune/geometry_resource_key.h"
#include <cstring>
#include <memory>
#include <unordered_map>
#include <utility>

namespace neptune {

// Validation-stage immutable payload cache. A representation key narrows
// lookup, but exact canonical bytes are authoritative until every baked UV
// dependency has a producer revision. Never overwrite a resident range.
template<std::size_t MaxRanges=4096, std::size_t MaxEntries=1024>
class ResidentGeometryCache {
public:
    using Arena=GeometryArena<MaxRanges>;
    using Range=typename Arena::Range;
    struct Stats {
        unsigned hits=0, uploads=0, mismatches=0, skipped=0, retired=0;
        std::uint64_t comparedBytes=0, uploadedBytes=0;
    };
    Range acquire(Arena& arena, unsigned char* base,
        const GeometryRepresentationKey& key, std::weak_ptr<const void> owner,
        const void* source, std::size_t bytes, std::uint64_t serial)
    {
        if (!base || !key.valid() || owner.expired() || !source || !bytes ||
            bytes>arena.capacity() || !serial) {
            ++stats_.skipped;
            return {};
        }
        auto it=entries_.find(key);
        if(it!=entries_.end()) {
            const bool sameOwner=!owner.owner_before(it->second.owner) &&
                !it->second.owner.owner_before(owner);
            const auto range=it->second.range;
            if(sameOwner && !it->second.owner.expired() && range.bytes==bytes) {
                stats_.comparedBytes+=bytes;
                if(std::memcmp(base+range.offset,source,bytes)==0) {
                    if(!arena.markUsed(range,serial)) {
                        ++stats_.skipped;
                        return {};
                    }
                    it->second.lastUse=serial;
                    ++stats_.hits;
                    return range;
                }
                ++stats_.mismatches;
            }
            // A changed payload is a new immutable allocation, even if the
            // old range was selected by a previous instance this transaction.
            if(!arena.retire(range)) {
                ++stats_.skipped;
                return {};
            }
            entries_.erase(it);
            ++stats_.retired;
        }
        if(entries_.size()>=MaxEntries) { ++stats_.skipped; return {}; }
        auto range=arena.allocate(bytes,64u);
        if(!range) { ++stats_.skipped; return {}; }
        std::memcpy(base+range.offset,source,bytes);
        if(!arena.markUsed(range,serial)) {
            arena.retire(range);
            ++stats_.skipped;
            return {};
        }
        entries_.emplace(key,Entry{range,std::move(owner),serial});
        ++stats_.uploads;
        stats_.uploadedBytes+=bytes;
        return range;
    }

    // Periodic bounded admission pressure relief. Current transaction
    // ranges are pinned even if producer ownership has already ended.
    void sweep(Arena& arena, std::uint64_t serial,
        std::uint64_t maxIdle=600, unsigned maxRetire=32)
    {
        if(!serial) return;
        unsigned count=0;
        for(auto it=entries_.begin();it!=entries_.end() && count<maxRetire;) {
            const auto& entry=it->second;
            if(entry.lastUse<serial &&
                (entry.owner.expired() || serial-entry.lastUse>maxIdle) &&
                arena.retire(entry.range)) {
                it=entries_.erase(it);
                ++stats_.retired;
                ++count;
            } else ++it;
        }
    }
    std::size_t size() const noexcept { return entries_.size(); }
    const Stats& stats() const noexcept { return stats_; }
    void resetStats() noexcept { stats_={}; }
    // Only after the consuming GPU context has been destroyed. The owner
    // then resets the entire arena; never use this as a live eviction API.
    void abandonAfterContextDestruction() { entries_.clear(); stats_={}; }

private:
    struct Entry {
        Range range;
        std::weak_ptr<const void> owner;
        std::uint64_t lastUse;
    };
    std::unordered_map<GeometryRepresentationKey,Entry,
        GeometryRepresentationKeyHash> entries_;
    Stats stats_{};
};
} // namespace neptune

#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <unordered_map>

namespace lagi {
// Native identity is opaque. The bridge's generation protects against
// recycled model addresses; never dereference a cached identity.
struct FieldPlacementKey {
    std::uintptr_t model=0;
    std::uint64_t generation=0;
    std::int8_t bundle=-1;
    std::uint32_t cell=0,object=0,offset=0;
    std::size_t vertices=0,polygons=0;
    std::array<std::int32_t,12> matrix{};
    bool operator==(const FieldPlacementKey& b) const {
        return model==b.model && generation==b.generation && bundle==b.bundle &&
            cell==b.cell && object==b.object && offset==b.offset &&
            vertices==b.vertices && polygons==b.polygons && matrix==b.matrix;
    }
};
struct FieldPlacementHash {
    std::size_t operator()(const FieldPlacementKey& k) const {
        // Exact equality above, even if the hash collides.
        std::uint32_t h=2166136261u;
        auto add=[&](std::uint32_t x){h=(h^x)*16777619u;};
        add(static_cast<std::uint32_t>(k.model));
        add(static_cast<std::uint32_t>(static_cast<std::uint64_t>(k.model)>>32));
        add(static_cast<std::uint32_t>(k.generation));add(static_cast<std::uint32_t>(k.generation>>32));
        add(static_cast<std::uint8_t>(k.bundle));add(k.cell);add(k.object);add(k.offset);
        add(static_cast<std::uint32_t>(k.vertices));add(static_cast<std::uint32_t>(k.polygons));
        for(auto x:k.matrix)add(static_cast<std::uint32_t>(x));
        return h;
    }
};
// Renderer-thread only. Budget counts exact retained vertex-array bytes;
// metadata is independently bounded by maxEntries. LRU work is confined to
// insertion under pressure, not hits. Cache owns no native/model/texture data.
template<class Vertex>
class FieldPlacementCache {
    struct Entry {
        std::unique_ptr<Vertex[]> payload;
        std::uint64_t used=0;
    };
    std::unordered_map<FieldPlacementKey,Entry,FieldPlacementHash> entries;
    std::size_t bytes_=0,maxBytes,maxEntries;
    std::uint64_t clock=0,evictions_=0;
public:
    explicit FieldPlacementCache(std::size_t byteBudget,std::size_t entryBudget)
        :maxBytes(byteBudget),maxEntries(entryBudget){}
    std::size_t bytes() const {return bytes_;}
    std::size_t size() const {return entries.size();}
    std::uint64_t evictions() const {return evictions_;}
    void clear(){entries.clear();bytes_=0;clock=0;evictions_=0;}
    const Vertex* find(const FieldPlacementKey& key) {
        auto it=entries.find(key);
        if(it==entries.end())return nullptr;
        it->second.used=++clock;
        return it->second.payload.get();
    }
    bool store(const FieldPlacementKey& key,const Vertex* data,std::size_t count) {
        if(!data || !key.model || !key.generation || !count ||
           count!=key.vertices || !maxEntries || count>maxBytes/sizeof(Vertex))
            return false;
        auto old=entries.find(key);
        if(old!=entries.end()){old->second.used=++clock;return true;}
        const std::size_t bytes=count*sizeof(Vertex);
        while(entries.size()>=maxEntries || bytes_>maxBytes-bytes) {
            auto victim=entries.begin();
            for(auto it=entries.begin();it!=entries.end();++it)
                if(it->second.used<victim->second.used)victim=it;
            bytes_-=victim->first.vertices*sizeof(Vertex);
            entries.erase(victim);++evictions_;
        }
        // Allocate only after eviction so payload memory never exceeds budget.
        // Value copy preserves each face's own color as well as position.
        std::unique_ptr<Vertex[]> payload(new Vertex[count]);
        for(std::size_t i=0;i<count;++i)payload[i]=data[i];
        entries.emplace(key,Entry{std::move(payload),++clock});
        bytes_+=bytes;
        return true;
    }
};
}

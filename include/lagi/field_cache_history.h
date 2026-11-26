#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <tuple>
#include <vector>

namespace lagi {
struct FieldCachePlacement {
    std::int8_t bundle=-1;
    std::uint32_t cell=0,object=0;
    auto key() const { return std::make_tuple(bundle,cell,object); }
};
struct FieldCacheVariant {
    FieldCachePlacement placement;
    std::uintptr_t model=0;
    std::uint64_t generation=0;
    std::uint32_t offset=0,polygons=0;
    std::array<std::int32_t,12> matrix{};
    bool hasMatrix=false;
    std::size_t order=0;
};
struct FieldCacheDelta {
    unsigned added=0,removed=0,lod=0,resource=0,transform=0;
    bool reordered=false,ambiguous=false;
};
// Only explicit native field placements are compared. Never infer a
// placement identity from a raw model pointer or classify arbitrary actors.
class FieldCacheHistory {
    std::vector<FieldCacheVariant> prior, current;
    bool ready=false,priorAmbiguous=false;
    static bool less(const FieldCacheVariant& a,const FieldCacheVariant& b) {
        return a.placement.key()<b.placement.key();
    }
public:
    bool initialized() const { return ready; }
    void reset() { prior.clear();current.clear();ready=false;priorAmbiguous=false; }
    void begin() { current.clear(); }
    void add(FieldCacheVariant v) { v.order=current.size();current.push_back(v); }
    FieldCacheDelta finish() {
        FieldCacheDelta d;
        // Compare in current submission order so insertion/removal does not
        // masquerade as relative reordering of surviving objects.
        std::sort(current.begin(),current.end(),less);
        const bool duplicate=std::adjacent_find(current.begin(),current.end(),
            [](const auto& a,const auto& b){return a.placement.key()==b.placement.key();})!=current.end();
        if(ready) {
            d.ambiguous=duplicate||priorAmbiguous;
            if(!d.ambiguous) {
                std::vector<std::pair<std::size_t,std::size_t>> common;
                common.reserve(std::min(prior.size(),current.size()));
                std::size_t a=0,b=0;
                while(a<prior.size()||b<current.size()) {
                    if(b==current.size()||(a<prior.size()&&less(prior[a],current[b]))) { ++d.removed;++a;continue; }
                    if(a==prior.size()||less(current[b],prior[a])) { ++d.added;++b;continue; }
                    const auto& p=prior[a++];const auto& c=current[b++];
                    if(p.offset!=c.offset) ++d.lod;
                    else if(p.model!=c.model||p.generation!=c.generation||p.polygons!=c.polygons) ++d.resource;
                    if(p.hasMatrix!=c.hasMatrix||(c.hasMatrix&&p.matrix!=c.matrix)) ++d.transform;
                    common.emplace_back(c.order,p.order);
                }
                std::sort(common.begin(),common.end());
                for(std::size_t i=1;i<common.size();++i)
                    if(common[i-1].second>common[i].second) { d.reordered=true;break; }
            }
        }
        prior.swap(current);ready=true;priorAmbiguous=duplicate;
        return d;
    }
};
}

#include "lagi/lagi_azel_upstream_prelude.h"
#include "lagi/resident_video.h"
#include "lagi/disc_image.h"
#include "lagi/lagi_render_bridge.h"
#include "lagi/platform.h"
#include <cstring>
#include <algorithm>
#include <map>

namespace lagi::resident_video {
namespace {
std::vector<Span> resident;
int residentDisc=-1;
struct Scope { std::vector<Range> ranges; std::vector<Span> spans; };
std::map<const void*,Scope> scopes;
struct Region { Space space; std::uint8_t* base; std::size_t size; };
Region region(Space space)
{
    switch(space){
    case Space::vdp1:return {space,getVdp1Pointer(0x25C00000),0x80000};
    case Space::vdp2:return {space,getVdp2Vram(0),0x80000};
    case Space::cram:return {space,getVdp2Cram(0),0x1000};
    }
    return {space,nullptr,0};
}
void synchronizeDisc()
{
    const int disc=lagi::disc::selected_disc();
    if(disc!=residentDisc){resident.clear();scopes.clear();residentDisc=disc;}
}
std::string key(const char* name)
{
    std::string value=name?name:"";
    for(auto& c:value)if(c>='a' && c<='z')c=static_cast<char>(c-'a'+'A');
    return value;
}
bool failed(Snapshot& snapshot,const std::string& message)
{
    if(snapshot.error!=message){
        lagi::platform::logging::writef("[BattleRestore] %s; field remains paused\n",message.c_str());
        snapshot.error=message;
    }
    return false;
}
} // namespace

void record_load(const char* filename,std::uint8_t* destination,std::size_t size)
{
    synchronizeDisc();
    if(!filename || !*filename || !destination || !size)return;
    for(const auto space:{Space::vdp1,Space::vdp2,Space::cram}){
        const auto owner=region(space);
        const auto address=reinterpret_cast<std::uintptr_t>(destination);
        const auto base=reinterpret_cast<std::uintptr_t>(owner.base);
        if(address<base || address-base>=owner.size)continue;
        const std::size_t begin=address-base;
        if(size>owner.size-begin)return;
        const std::size_t end=begin+size;
        std::vector<Span> next;
        next.reserve(resident.size()+2);
        for(const auto& old:resident){
            const auto oldEnd=old.destination+old.length;
            if(old.space!=space || oldEnd<=begin || old.destination>=end){next.push_back(old);continue;}
            if(old.destination<begin){auto left=old;left.length=begin-old.destination;next.push_back(left);}
            if(oldEnd>end){auto right=old;right.source+=end-old.destination;
                right.destination=end;right.length=oldEnd-end;next.push_back(right);}
        }
        next.push_back({key(filename),space,begin,0,size,size});
        resident.swap(next);
        if(space==Space::vdp1 || space==Space::cram)
            lagi::azel_bridge::notify_native_texture_write();
        return;
    }
}

void capture(Snapshot& snapshot)
{
    synchronizeDisc();
    snapshot={};snapshot.disc=residentDisc;snapshot.spans=resident;
}

bool prepare(Snapshot& snapshot)
{
    if(snapshot.disc!=lagi::disc::selected_disc())return failed(snapshot,"selected disc changed");
    if(snapshot.prepared)return true;
    snapshot.files.clear();
    for(const auto& span:snapshot.spans){
        auto found=std::find_if(snapshot.files.begin(),snapshot.files.end(),
            [&](const PreparedFile& file){return file.filename==span.filename;});
        if(found==snapshot.files.end()){
            PreparedFile file;file.filename=span.filename;
            if(!lagi::disc::read_file(file.filename.c_str(),file.bytes))return failed(snapshot,"cannot reload "+file.filename);
            snapshot.files.push_back(std::move(file));found=snapshot.files.end()-1;
        }
        const auto owner=region(span.space);
        if(found->bytes.size()!=span.fileSize || span.source>span.fileSize ||
            span.length>span.fileSize-span.source || span.destination>owner.size ||
            span.length>owner.size-span.destination)return failed(snapshot,"size/range mismatch for "+span.filename);
    }
    snapshot.prepared=true;
    snapshot.error.clear();
    return true;
}

bool restore(Snapshot& snapshot)
{
    if(!snapshot.prepared || snapshot.disc!=lagi::disc::selected_disc())return false;
    for(const auto& span:snapshot.spans){
        const auto found=std::find_if(snapshot.files.begin(),snapshot.files.end(),
            [&](const PreparedFile& file){return file.filename==span.filename;});
        const auto owner=region(span.space);
        if(found==snapshot.files.end() || found->bytes.size()!=span.fileSize ||
            span.source>span.fileSize || span.length>span.fileSize-span.source ||
            span.destination>owner.size || span.length>owner.size-span.destination)return false;
    }
    for(const auto& span:snapshot.spans){
        const auto found=std::find_if(snapshot.files.begin(),snapshot.files.end(),
            [&](const PreparedFile& file){return file.filename==span.filename;});
        std::memcpy(region(span.space).base+span.destination,found->bytes.data()+span.source,span.length);
    }
    resident=snapshot.spans;residentDisc=snapshot.disc;
    // Publication already snapshots native memory once per write epoch.
    // Mark the restored bytes dirty instead of mutating renderer caches here.
    lagi::azel_bridge::notify_native_texture_write();
    return true;
}

void save_scope(const void* owner,std::initializer_list<Range> ranges)
{
    synchronizeDisc();
    if(!owner || scopes.find(owner)!=scopes.end())return;
    Scope scope;
    std::vector<Range> sorted=ranges;
    std::sort(sorted.begin(),sorted.end(),[](const Range& a,const Range& b){
        return a.space==b.space?a.destination<b.destination:a.space<b.space;
    });
    for(const auto& range:sorted){
        const auto memory=region(range.space);
        if(range.destination>memory.size || range.length>memory.size-range.destination)return;
        if(!scope.ranges.empty()){
            auto& previous=scope.ranges.back();
            if(previous.space==range.space && previous.destination+previous.length>=range.destination){
                previous.length=std::max(previous.destination+previous.length,range.destination+range.length)-previous.destination;
                continue;
            }
        }
        scope.ranges.push_back(range);
    }
    for(const auto& span:resident)for(const auto& range:scope.ranges){
        if(span.space!=range.space)continue;
        const auto begin=std::max(span.destination,range.destination);
        const auto end=std::min(span.destination+span.length,range.destination+range.length);
        if(begin>=end)continue;
        auto saved=span;saved.source+=begin-span.destination;
        saved.destination=begin;saved.length=end-begin;scope.spans.push_back(std::move(saved));
    }
    scopes.emplace(owner,std::move(scope));
}

void restore_scope(const void* owner)
{
    synchronizeDisc();
    const auto found=scopes.find(owner);
    if(found==scopes.end())return;
    for(const auto& range:found->second.ranges){
        std::vector<Span> next;
        for(const auto& span:resident){
            const auto begin=range.destination,end=begin+range.length;
            const auto oldEnd=span.destination+span.length;
            if(span.space!=range.space || oldEnd<=begin || span.destination>=end){next.push_back(span);continue;}
            if(span.destination<begin){auto left=span;left.length=begin-span.destination;next.push_back(left);}
            if(oldEnd>end){auto right=span;right.source+=end-span.destination;
                right.destination=end;right.length=oldEnd-end;next.push_back(right);}
        }
        resident.swap(next);
    }
    resident.insert(resident.end(),found->second.spans.begin(),found->second.spans.end());
    scopes.erase(found);
}
} // namespace lagi::resident_video

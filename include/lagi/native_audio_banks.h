#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <algorithm>
#include <cstddef>
#include <cstring>
#include <utility>

namespace lagi {
constexpr int native_audio_load_state(int previous,int previousArea,int next,int nextArea)
{
    return previous<0 || previousArea!=nextArea ? 0 : previous==next ? 8 : 2;
}
constexpr bool native_audio_bank_playing(std::uint16_t status){return (status&0xFF00u)!=0;}
// AREAMAP.SND: 8-byte entries (type/bank, BE24 address, BE32 size),
// one 0xFF terminator per map, then a final 0xFF terminator for the table.
// Type 3 describes the writable DSP ring/work area, not immutable disc data.
struct NativeAudioBankResidency {
    static constexpr std::uint32_t ramBase=0x25A00000u;
    static constexpr std::uint32_t ramBytes=0x80000u;
    struct Range{std::uint32_t address;std::size_t size;};
    struct File{
        std::string name;std::uint32_t address;std::size_t size;
        std::vector<std::uint8_t> originalDsp;
    };
    std::vector<File> files;
    std::vector<std::vector<Range>> areaMaps;
    int disc=-1,activeArea=-1;
    bool mapsValid=false;

    static bool overlaps(std::uint32_t a,std::size_t an,std::uint32_t b,std::size_t bn){
        return an && bn && std::uint64_t(a)<std::uint64_t(b)+bn &&
            std::uint64_t(b)<std::uint64_t(a)+an;
    }
    static std::string key(const std::string& name){
        auto result=name;for(auto& c:result)if(c>='a' && c<='z')c=char(c-'a'+'A');return result;
    }
    static bool is_dsp(const std::string& name){
        const auto normalized=key(name);
        return normalized.size()>=4 && normalized.compare(normalized.size()-4,4,".EXB")==0;
    }
    void select_disc(int next){
        if(next!=disc){files.clear();disc=next;}
    }
    bool set_area_maps(const std::uint8_t* bytes,std::size_t size){
        areaMaps.clear();mapsValid=false;activeArea=-1;
        if(!bytes)return false;
        std::vector<std::vector<Range>> parsed;
        std::size_t i=0;
        while(i<size && bytes[i]!=0xFF){
            std::vector<Range> ranges;
            while(i<size && bytes[i]!=0xFF){
                if(size-i<8)return false;
                const auto* p=bytes+i;
                const std::uint32_t start=(std::uint32_t(p[1])<<16)|(std::uint32_t(p[2])<<8)|p[3];
                const std::uint32_t length=(std::uint32_t(p[4])<<24)|(std::uint32_t(p[5])<<16)|(std::uint32_t(p[6])<<8)|p[7];
                if((p[0]&0xF0)==0x30){
                    if(!length || start>=ramBytes || length>ramBytes-start)return false;
                    ranges.push_back({ramBase+start,length});
                }
                i+=8;
            }
            if(i==size || ranges.empty())return false;
            ++i;parsed.push_back(ranges);
        }
        if(i==size || parsed.empty())return false;
        areaMaps.swap(parsed);mapsValid=true;return true;
    }
    std::size_t invalidate(std::uint32_t address,std::size_t size){
        const auto before=files.size();
        files.erase(std::remove_if(files.begin(),files.end(),[&](const File& old){
            return overlaps(address,size,old.address,old.size);
        }),files.end());
        return before-files.size();
    }
    bool known_area(int area)const{
        return mapsValid && area>=0 && std::size_t(area)<areaMaps.size();
    }
    // Invoke before issuing the driver's area-map command. A future file copy
    // must not rely on an identity that the new DSP work area can overwrite.
    std::size_t activate_area(int area){
        activeArea=area;
        const auto before=files.size();
        if(!known_area(area))files.clear();
        else for(const auto& r:areaMaps[std::size_t(area)])invalidate(r.address,r.size);
        return before-files.size();
    }
    bool writable(std::uint32_t address,std::size_t size)const{
        if(activeArea<0)return false; // boot banks before first area-map command
        if(!known_area(activeArea))return true;
        for(const auto& r:areaMaps[std::size_t(activeArea)])
            if(overlaps(address,size,r.address,r.size))return true;
        return false;
    }
    bool contains(const std::string& name,std::uint32_t address)const{
        const auto normalized=key(name);
        for(const auto& file:files)if(file.address==address && file.name==normalized)return true;
        return false;
    }
    // Only DSP executables require exact-byte checking on cache hits. This
    // avoids re-reading large sample banks or scanning RAM in the render loop.
    bool verify(const std::string& name,std::uint32_t address,const std::uint8_t* ram,std::size_t bytes){
        const auto normalized=key(name);
        for(auto it=files.begin();it!=files.end();++it){
            if(it->address!=address || it->name!=normalized)continue;
            if(!is_dsp(normalized))return true;
            if(ram && address>=ramBase && address-ramBase<=bytes &&
                it->size<=bytes-(address-ramBase) && it->originalDsp.size()==it->size &&
                it->size && !std::memcmp(ram+(address-ramBase),it->originalDsp.data(),it->size))return true;
            files.erase(it);return false;
        }
        return false;
    }
    void loaded(const std::string& name,std::uint32_t address,std::size_t size,const std::uint8_t* data=nullptr){
        invalidate(address,size);
        if(writable(address,size))return;
        File file{key(name),address,size,{}};
        if(data && is_dsp(name))file.originalDsp.assign(data,data+size);
        files.push_back(std::move(file));
    }
};
}

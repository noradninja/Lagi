#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <algorithm>

namespace lagi {
constexpr int native_audio_load_state(int previous,int previousArea,int next,int nextArea)
{
    return previous<0 || previousArea!=nextArea ? 0 : previous==next ? 8 : 2;
}
constexpr bool native_audio_bank_playing(std::uint16_t status){return (status&0xFF00u)!=0;}
struct NativeAudioBankResidency {
    struct File{std::string name;std::uint32_t address;std::size_t size;};
    std::vector<File> files;
    int disc=-1;
    void select_disc(int next){if(next!=disc){files.clear();disc=next;}}
    static std::string key(const std::string& name){
        auto result=name;for(auto& c:result)if(c>='a' && c<='z')c=char(c-'a'+'A');return result;
    }
    bool contains(const std::string& name,std::uint32_t address)const{
        const auto normalized=key(name);
        for(const auto& file:files)if(file.address==address && file.name==normalized)return true;
        return false;
    }
    void loaded(const std::string& name,std::uint32_t address,std::size_t size){
        const auto end=std::uint64_t(address)+size;
        files.erase(std::remove_if(files.begin(),files.end(),[&](const File& old){
            return std::uint64_t(old.address)<end && std::uint64_t(address)<std::uint64_t(old.address)+old.size;
        }),files.end());
        files.push_back({key(name),address,size});
    }
};
}

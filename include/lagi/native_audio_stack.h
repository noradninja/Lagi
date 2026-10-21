#pragma once
#include <array>
#include <cstdint>

namespace lagi {
template<class Sounds> struct NativeAudioStack {
    struct Entry { int sequence=-1,mode=1; bool marked=false; Sounds sounds{}; };
    std::array<Entry,8> entries{};
    unsigned index=0;
    void mark(){entries[index].marked=true;}
    // Original 0602B29A clears the parent's mark (upstream incorrectly sets 1).
    // Existing sound IDs are retained; the native volume sentinel is reset.
    bool load(int sequence,int mode,int returnOffset,const Sounds& sounds){
        if(entries[index].marked){
            if(index==7)return false;
            auto& parent=entries[index];parent.marked=false;
            parent.sequence+=returnOffset;parent.sounds=sounds;
            for(auto& sound:parent.sounds)sound.volume=0x80;
            ++index;entries[index]={};
            entries[index].sounds=parent.sounds;
        }
        auto& current=entries[index];current.sequence=sequence;current.mode=mode;current.marked=false;
        return true;
    }
    enum class Pop { canceled,empty,restore };
    Pop pop(){
        if(entries[index].marked){entries[index].marked=false;return Pop::canceled;}
        if(!index)return Pop::empty;
        entries[index].sequence=-1;entries[index].marked=false;--index;
        return Pop::restore;
    }
};
}

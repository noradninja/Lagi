#pragma once
#include <cstddef>
namespace lagi {
// Platform loading cover only: no fade clock, task updates or encounter choice.
struct BattleLoadingCover {
    bool active=false,loading=false;
    void begin(){active=true;loading=false;}
    bool allowLoad(bool stopped,int red,int green,int blue){
        if(!active)return true;
        if(!stopped || red<255 || green<255 || blue<255)return false;
        loading=true;return true;
    }
    bool retainPreviousScene(std::size_t submissions,bool engineReady=true){
        if(active && loading && engineReady && submissions)active=false;
        return active;
    }
};
inline BattleLoadingCover battleLoadingCover;
}

#pragma once
#include <cstddef>
#include <cstdint>
// Returns 0 only after the complete packet has been copied, -1 for retry,
// -2 for unsupported/malformed metadata. Never retains the input pointer.
int lagiSubmitCutscenePCM(const void* owner,const std::uint8_t* data,std::size_t bytes,unsigned rate,unsigned channels,unsigned bits,
    const std::uint8_t* wrapped=nullptr,std::size_t contiguous=~std::size_t(0));
void lagiStopCutscenePCM(const void* owner);
void lagiPauseCutscenePCM(const void* owner,bool paused);

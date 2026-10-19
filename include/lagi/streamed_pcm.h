#pragma once
#include <array>
#include <atomic>
#include <cstdint>
#include <cstddef>
#include <algorithm>

namespace lagi {
// One producer (Azel packet parser), one consumer (existing audio worker).
// Generation acknowledgement makes reset/skip safe without resetting another
// thread's cursor or retaining pointers into Azel's recyclable command ring.
template<std::size_t Capacity=65536> class StreamedPCM {
    static_assert(Capacity>1 && (Capacity&(Capacity-1))==0);
    std::array<std::int16_t,Capacity*2> samples{};
    std::atomic<std::uint32_t> write{0},read{0},request{0},acknowledged{0},requestedRate{0};
    std::atomic<bool> paused{false};
    std::uint32_t generation=0,rate=0,phase=0;
public:
    enum class Result { accepted,busy,invalid };
    void begin(std::uint32_t sampleRate) {
        paused.store(false,std::memory_order_release);
        requestedRate.store(sampleRate,std::memory_order_relaxed);
        request.fetch_add(1,std::memory_order_release);
    }
    Result enqueue(const std::uint8_t* data,std::size_t bytes,unsigned channels,unsigned bits,
                   const std::uint8_t* wrapped=nullptr,std::size_t contiguous=~std::size_t(0)) {
        if(!data || (channels!=1&&channels!=2) || (bits!=8&&bits!=16))return Result::invalid;
        if(contiguous<bytes && !wrapped)return Result::invalid;
        const unsigned width=bits/8;
        if(bytes%(channels*width))return Result::invalid;
        const std::size_t frames=bytes/(channels*width);
        if(frames>Capacity)return Result::invalid;
        if(acknowledged.load(std::memory_order_acquire)!=request.load(std::memory_order_acquire))return Result::busy;
        const auto w=write.load(std::memory_order_relaxed),r=read.load(std::memory_order_acquire);
        if(frames>Capacity-std::uint32_t(w-r))return Result::busy;
        const auto byte=[&](std::size_t at){return at<contiguous?data[at]:wrapped[at-contiguous];};
        // Original 0600ED20 uses left/right planes, not interleaved input.
        for(std::size_t i=0;i<frames;++i)for(unsigned ch=0;ch<2;++ch) {
            const auto at=((channels==2?ch:0)*frames+i)*width;
            const int value=bits==16 ? int(std::int16_t((unsigned(byte(at))<<8)|byte(at+1))) :
                (int(byte(at))>=128?int(byte(at))-256:int(byte(at)))*256;
            samples[((w+std::uint32_t(i))&(Capacity-1))*2+ch]=std::int16_t(value);
        }
        write.store(w+std::uint32_t(frames),std::memory_order_release);
        return Result::accepted;
    }
    void mix(std::int16_t* output,std::size_t frames,unsigned outputRate) {
        const auto wanted=request.load(std::memory_order_acquire);
        if(generation!=wanted) {
            read.store(write.load(std::memory_order_acquire),std::memory_order_release);
            phase=0;rate=requestedRate.load(std::memory_order_relaxed);generation=wanted;
            acknowledged.store(wanted,std::memory_order_release);
        }
        if(!rate||!outputRate||paused.load(std::memory_order_acquire))return;
        auto r=read.load(std::memory_order_relaxed);
        const auto w=write.load(std::memory_order_acquire);
        for(std::size_t i=0;i<frames && r!=w;++i) {
            const auto next=r+1==w?r:r+1;
            for(unsigned ch=0;ch<2;++ch) {
                const int a=samples[(r&(Capacity-1))*2+ch],b=samples[(next&(Capacity-1))*2+ch];
                const int pcm=a+int((std::int64_t(b-a)*phase)/outputRate);
                output[i*2+ch]=std::int16_t(std::clamp(int(output[i*2+ch])+pcm,-32768,32767));
            }
            const auto advance=(std::uint64_t(phase)+rate)/outputRate;
            phase=std::uint32_t((std::uint64_t(phase)+rate)%outputRate);
            r+=std::uint32_t(std::min<std::uint64_t>(advance,std::uint32_t(w-r)));
        }
        read.store(r,std::memory_order_release);
    }
    std::uint32_t queued()const{const auto r=read.load(std::memory_order_acquire);return write.load(std::memory_order_acquire)-r;}
    void pause(bool value){paused.store(value,std::memory_order_release);}
};
}

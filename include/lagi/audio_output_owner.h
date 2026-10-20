#pragma once
#include <atomic>
namespace lagi::audio {
// CPK owns the physical output port; native EPK PCM shares the SCSP mixer.
// Sequence stop/fade commands do not transfer ownership of that port.
inline std::atomic<bool> movieOwnsOutput{false};
inline std::atomic_flag outputLock=ATOMIC_FLAG_INIT;
struct OutputLease {
    OutputLease(){while(outputLock.test_and_set(std::memory_order_acquire)) {}}
    ~OutputLease(){outputLock.clear(std::memory_order_release);}
};
inline void claim_movie_output() {
    movieOwnsOutput.store(true,std::memory_order_release);
    OutputLease waitForNativeQuantum;
}
inline void release_movie_output(){movieOwnsOutput.store(false,std::memory_order_release);}
}

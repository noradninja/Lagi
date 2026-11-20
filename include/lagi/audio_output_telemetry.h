#pragma once
#include <atomic>
#include <cstdint>

namespace lagi::audio {

// The output thread only updates counters; it never logs or takes timestamps.
struct OutputQueueWindow {
    std::uint64_t blocks = 0, shortBlocks = 0, emptyBlocks = 0, zeroFrames = 0;
    unsigned minQueued = 0, maxQueued = 0, maxMissing = 0;
};

class OutputQueueTelemetry {
    std::atomic<std::uint64_t> blocks_{0}, short_{0}, empty_{0}, zeros_{0};
    std::atomic<unsigned> low_{~0u}, high_{0}, worst_{0};
    static void min_update(std::atomic<unsigned>& a, unsigned n) noexcept {
        unsigned v = a.load(std::memory_order_relaxed);
        while (n < v && !a.compare_exchange_weak(v,n,std::memory_order_relaxed)) {}
    }
    static void max_update(std::atomic<unsigned>& a, unsigned n) noexcept {
        unsigned v = a.load(std::memory_order_relaxed);
        while (n > v && !a.compare_exchange_weak(v,n,std::memory_order_relaxed)) {}
    }
public:
    void clear() noexcept {
        blocks_.store(0); short_.store(0); empty_.store(0); zeros_.store(0);
        low_.store(~0u); high_.store(0); worst_.store(0);
    }
    void record(unsigned queued, unsigned blockSize) noexcept {
        if (!blockSize) return;
        blocks_.fetch_add(1,std::memory_order_relaxed);
        min_update(low_,queued);
        max_update(high_,queued);
        if (queued >= blockSize) return;
        const unsigned missing = blockSize - queued;
        short_.fetch_add(1,std::memory_order_relaxed);
        if (!queued) empty_.fetch_add(1,std::memory_order_relaxed);
        zeros_.fetch_add(missing,std::memory_order_relaxed);
        max_update(worst_,missing);
    }
    OutputQueueWindow take_interval() noexcept {
        OutputQueueWindow w{};
        w.blocks = blocks_.exchange(0);
        w.shortBlocks = short_.exchange(0);
        w.emptyBlocks = empty_.exchange(0);
        w.zeroFrames = zeros_.exchange(0);
        const unsigned low = low_.exchange(~0u);
        w.minQueued = low == ~0u ? 0 : low;
        w.maxQueued = high_.exchange(0);
        w.maxMissing = worst_.exchange(0);
        return w;
    }
};

extern OutputQueueTelemetry outputQueueTelemetry;
} // namespace lagi::audio

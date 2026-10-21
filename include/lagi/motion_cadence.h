#pragma once
#include <array>
#include <cstdint>
#include <algorithm>

namespace lagi {
// Diagnostic only. Exact snapshots, not hashes; no movement or timing control.
template<unsigned A, unsigned B> struct MotionCadence {
    struct Summary {
        unsigned intervals=0, aRepeated=0, bRepeated=0, aChangedBRepeated=0;
        unsigned serialGaps=0;
        std::uint64_t minUs=UINT64_MAX, maxUs=0, totalUs=0;
        std::uint64_t minBStep=UINT64_MAX, maxBStep=0;
    } summary;
    std::array<std::int32_t,A> previousA{};
    std::array<std::int32_t,B> previousB{};
    std::uint64_t lastUs=0, lastSerial=0;
    unsigned key=0;
    bool valid=false;
    bool add(unsigned nextKey, std::uint64_t serial, std::uint64_t nowUs,
             const std::array<std::int32_t,A>& a,
             const std::array<std::int32_t,B>& b) {
        if(!valid || key!=nextKey || nowUs<lastUs) {
            summary={};valid=true;key=nextKey;
        } else {
            const auto elapsed=nowUs-lastUs;
            ++summary.intervals;summary.totalUs+=elapsed;
            summary.minUs=std::min(summary.minUs,elapsed);
            summary.maxUs=std::max(summary.maxUs,elapsed);
            const bool sameA=a==previousA,sameB=b==previousB;
            summary.aRepeated+=sameA;summary.bRepeated+=sameB;
            summary.aChangedBRepeated+=!sameA && sameB;
            summary.serialGaps+=serial!=lastSerial+1;
            std::uint64_t step=0;
            for(unsigned i=0;i<B;++i) {
                const auto delta=std::int64_t(b[i])-previousB[i];
                step+=delta<0?std::uint64_t(-delta):std::uint64_t(delta);
            }
            summary.minBStep=std::min(summary.minBStep,step);
            summary.maxBStep=std::max(summary.maxBStep,step);
        }
        previousA=a;previousB=b;lastUs=nowUs;lastSerial=serial;
        return summary.intervals==120;
    }
    // Keep endpoint, so inter-window logging time remains visible.
    void nextWindow(){summary={};}
    void reset(){valid=false;summary={};}
};
} // namespace lagi

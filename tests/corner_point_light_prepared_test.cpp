#include "lagi/lagi_corner_point_light.h"
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <random>

// Previous per-source implementation, retained here as an independent parity
// oracle for the prepared-corner path. Do not replace it with the new helper.
static bool previous_evaluate(const float world[3],
    const std::int32_t params[5], const std::uint16_t color[3],
    const std::int32_t basis[9], const std::int32_t* view,
    lagi::AdditionalModelLight& out)
{
    std::int32_t matrix[12]{0x10000,0,0,0, 0,0x10000,0,0, 0,0,0x10000,0};
    for (unsigned row=0;row<3;++row) {
        double v = view ? double(view[row*4+3]) : 0.0;
        for (unsigned col=0;col<3;++col)
            v += (view ? double(view[row*4+col]) : (row==col ? 65536.0:0.0))
                * double(world[col]);
        matrix[row*4+3]=static_cast<std::int32_t>(std::clamp(std::round(v),
            double(std::numeric_limits<std::int32_t>::min()),
            double(std::numeric_limits<std::int32_t>::max())));
    }
    const std::uint64_t maxSquared=std::uint64_t(std::uint32_t(params[3]))<<16;
    std::uint64_t distanceSquared=0;
    for(unsigned axis=0;axis<3;++axis) {
        const std::int64_t delta=std::int64_t(params[axis])-matrix[axis*4+3];
        if(delta < -6000000 || delta > 6000000) return false;
        distanceSquared+=std::uint64_t(delta*delta);
        if(distanceSquared>maxSquared) return false;
    }
    const auto result=lagi::native_point_light(matrix,params);
    if(result.outside || result.divisionOverflow) return false;
    out={};
    out.active=true;
    for(unsigned axis=0;axis<3;++axis) {
        std::int64_t rotated=0;
        for(unsigned row=0;row<3;++row) {
            std::int64_t modelColumn=0;
            for(unsigned col=0;col<3;++col)
                modelColumn+=
                    (view ? std::int64_t(view[row*4+col]) :
                        (row==col ? 65536LL : 0LL)) *
                    basis[col*3+axis];
            rotated+=(modelColumn>>16)*result.vector[row];
        }
        out.vector[axis]=static_cast<std::int16_t>(rotated>>16);
        out.color[axis]=color[axis];
    }
    return true;
}

int main()
{
    std::mt19937 rng(0xA2315u);
    const double roundingCases[]={
        -2147483649.0,-2147483648.5,-2147483648.0,
        -1.5,-0.5,std::nextafter(-0.5,0.0),-0.0,
        std::nextafter(0.5,0.0),0.5,1.5,
        2147483646.5,2147483647.0,2147483647.5,
        std::numeric_limits<double>::infinity(),
        -std::numeric_limits<double>::infinity()};
    for (const double value:roundingCases) {
        const auto expected=static_cast<std::int32_t>(std::clamp(
            std::round(value),
            double(std::numeric_limits<std::int32_t>::min()),
            double(std::numeric_limits<std::int32_t>::max())));
        if (lagi::round_clamped_point_word(value)!=expected) {
            std::printf("FAIL: point word rounding %.17g\n",value);
            return 1;
        }
    }
    std::uniform_int_distribution<int> position(-80,80);
    std::uniform_int_distribution<int> basisValue(-65536,65536);
    std::uniform_int_distribution<int> colorValue(0,31);
    unsigned hits=0, misses=0;
    for (unsigned i=0;i<20000;++i) {
        const float world[3]={float(position(rng))/4.0f,
            float(position(rng))/4.0f,float(position(rng))/4.0f};
        std::int32_t params[5]={position(rng)*16384,
            position(rng)*16384,position(rng)*16384,
            static_cast<std::int32_t>((i%3u==0u ? 0x5000u : 0x14000u)<<12),
            0x19000};
        const std::uint16_t color[3]={static_cast<std::uint16_t>(colorValue(rng)),
            static_cast<std::uint16_t>(colorValue(rng)),
            static_cast<std::uint16_t>(colorValue(rng))};
        std::int32_t basis[9];
        for (auto& entry:basis) entry=basisValue(rng);
        std::int32_t view[12]{65536,0,0,0,0,65536,0,0,0,0,65536,0};
        if (i%2u) {
            for (unsigned row=0;row<3;++row) {
                for (unsigned col=0;col<3;++col)
                    view[row*4+col]=basisValue(rng);
                view[row*4+3]=position(rng)*4096;
            }
        }
        const auto* selectedView=(i%2u) ? view : nullptr;
        lagi::AdditionalModelLight before{}, after{};
        const bool oldHit=previous_evaluate(world,params,color,basis,
            selectedView,before);
        std::int32_t matrix[12];
        lagi::prepare_point_corner_matrix(world,selectedView,matrix);
        const bool newHit=lagi::evaluate_point_at_corner_prepared(matrix,params,
            color,basis,selectedView,after);
        const auto preparedBasis=lagi::prepare_point_corner_basis(basis,selectedView);
        lagi::AdditionalModelLight reused{};
        const bool reusedHit=lagi::evaluate_point_at_corner_prepared_basis(
            matrix,params,color,preparedBasis,reused);
        if (oldHit!=newHit || (oldHit &&
            (before.active!=after.active ||
             !std::equal(std::begin(before.vector),std::end(before.vector),
                 std::begin(after.vector)) ||
             !std::equal(std::begin(before.color),std::end(before.color),
                 std::begin(after.color)))) ||
            newHit!=reusedHit || (newHit &&
            (after.active!=reused.active ||
             !std::equal(std::begin(after.vector),std::end(after.vector),
                 std::begin(reused.vector)) ||
             !std::equal(std::begin(after.color),std::end(after.color),
                 std::begin(reused.color))))) {
            std::printf("FAIL: prepared point corner case %u\n",i);
            return 1;
        }
        if (oldHit) ++hits; else ++misses;
    }
    std::printf("PASS: %u hits and %u misses match previous corner evaluation\n",
        hits,misses);
}

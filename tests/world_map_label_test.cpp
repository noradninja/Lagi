#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
using s16=std::int16_t;
struct fixedPoint {int value;int asS32()const{return value;}};
struct sVec3_FP {std::array<fixedPoint,3> values;const fixedPoint& operator[](int i)const{return values[i];}};
#include "lagi/world_map_label.h"
struct {struct {fixedPoint m18_widthScale{176},m1C_heightScale{112};} m405C;} graphicEngineStatus;
int transforms=0,divides=0,multiplies=0;
void transformAndAddVecByCurrentMatrix(const sVec3_FP* position,sVec3_FP* view){
    assert(position->values[0].value==7);++transforms;
    view->values[0]={3};view->values[1]={4};view->values[2]={0x20000};
}
fixedPoint FP_Div(int numerator,fixedPoint denominator){
    assert(numerator==0x10000&&denominator.value==0x20000);++divides;return {0x8000};
}
fixedPoint MTH_Mul_5_6(fixedPoint scale,fixedPoint axis,fixedPoint reciprocal){
    assert(reciprocal.value==0x8000);
    if(!(multiplies++&1)){assert(scale.value==176&&axis.value==3);return {50};}
    assert(scale.value==112&&axis.value==4);return {24};
}
#include "production.inc"
int main(){
    using namespace lagi::worldmap;
    sVec3_FP position{};position.values[0]={7};
    auto place=project_map_label(position,20);
    assert(transforms==1&&divides==1&&multiplies==2);
    assert(place.x==30&&place.y==21&&place.nativeZ==0x333);
    unsigned comparisons=0;
    for(int half:{0,1,20,80,160,200})
    for(int x:{-513,-512,-161,-160,-1,0,159,160,511,512,513})
    for(int y:{-513,-512,-89,-88,0,104,105,511,512,513}){
        const auto actual=bound_map_label(x,y,s16(half));
        std::int64_t expectedX=std::int64_t(x)-half,expectedY=std::int64_t(y)-3;
        if(expectedX>512||expectedX< -512||expectedY>512||expectedY< -512){expectedX=0;expectedY=-256;}
        else{
            if(expectedX< -160)expectedX=-160;
            if(expectedX>160-2*half)expectedX=160-2*half;
            if(expectedY>104)expectedY=104;
            if(expectedY< -88)expectedY=-88;
        }
        assert(actual.x==expectedX&&actual.y==expectedY&&actual.nativeZ==0x333);++comparisons;
    }
    // Outlier rejection observes SH-2 low-word wrapping before comparisons.
    auto wrapped=bound_map_label(INT32_MIN,INT32_MAX,1);
    assert(wrapped.x==0&&wrapped.y==-256);
    std::printf("PASS native label service bindings and %u independent bounds comparisons; sprite-text cutover is not yet active\n",comparisons);
}

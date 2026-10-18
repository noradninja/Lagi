#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
using s32=int32_t; using s16=int16_t; using s8=int8_t;
using u8=uint8_t; using u16=uint16_t;
struct fixedPoint {
    s32 raw=0;
    fixedPoint(s32 x=0):raw(x){}
    static fixedPoint fromInteger(s32 x){return fixedPoint(x*65536);}
    s32 toInteger()const{return raw/65536;}
    fixedPoint& operator+=(fixedPoint x){raw+=x.raw;return *this;}
    friend fixedPoint operator-(fixedPoint a){return -a.raw;}
    friend fixedPoint operator-(fixedPoint a,fixedPoint b){return a.raw-b.raw;}
    friend bool operator<(fixedPoint a,fixedPoint b){return a.raw<b.raw;}
    friend bool operator>(fixedPoint a,fixedPoint b){return a.raw>b.raw;}
    friend bool operator<=(fixedPoint a,fixedPoint b){return a.raw<=b.raw;}
    friend bool operator==(fixedPoint a,fixedPoint b){return a.raw==b.raw;}
};
fixedPoint FP_Div(fixedPoint a,fixedPoint b){return s32((int64_t(a.raw)*65536)/b.raw);}
using sVec3_FP=std::array<fixedPoint,3>;
struct Task {bool finished=false;void markFinished(){finished=true;}};
struct s_workArea { virtual ~s_workArea()=default; unsigned char prefix[97]{};Task task;Task* getTask(){return &task;} };
using p_workArea=s_workArea*;
template<class T> struct s_workAreaTemplate:s_workArea{};
#include "battle/battleEnemyLifeMeter.h"
struct Actor {p_workArea mC8,m40_enemyLifeMeterTask,mAC_lifeMeter;};
#include "signals.inc"
struct Debug {std::array<int,32> mFlags{};} debug;
struct Engine {int m474_XPReceivedFromBattle=0,m478_dyneReceivedFromBattle=0;} engine;
struct Overlay {Debug* m10_inBattleDebug=&debug;Engine* m4_battleEngine=&engine;} overlay;
struct Manager {Overlay* m10_battleOverlay=&overlay;} manager;
auto* gBattleManager=&manager;
struct Common {int getSaturnPtr(int x){return x;}} common;
auto* gCommonFile=&common;
s16 readSaturnS16(int address){return ((address-0x201798)%8)==0?17:9;}
void vdp2DebugPrintSetPosition(int,int){}
template<class... T> void vdp2PrintfSmallFont(const char*,T...){}
int performModulo(int a,int b){return b%a;}
int performModulo2(int a,int b){return b%a;}
int randomNumber(){assert(false);return 0;}
#include "update.inc"
int main(){
    for(unsigned i=0;i<std::size(masks);++i){
        sEnemyLifeMeterTask meter{};
        std::array<unsigned char,sizeof(meter)> before{};
        std::memcpy(before.data(),&meter,sizeof(meter));
        signal(&meter,i);
        assert(meter.m31==masks[i]);
        const auto offset=reinterpret_cast<unsigned char*>(&meter.m31)-reinterpret_cast<unsigned char*>(&meter);
        assert(offset!=0x31);
        for(unsigned j=0;j<sizeof(meter);++j)if(j!=offset)
            assert(before[j]==reinterpret_cast<unsigned char*>(&meter)[j]);
    }
    for(unsigned i=0;i+1<std::size(masks);++i)if(masks[i]==1&&masks[i+1]==8){
        sEnemyLifeMeterTask meter{};sVec3_FP position{};position[0]=123;
        s16 hp=0;meter.m0=&position;meter.m24=&hp;
        meter.m1C=fixedPoint::fromInteger(500);meter.m2E_width=50;
        meter.m2C_entryIndex=2;meter.m2A=30;
        engine={};signal(&meter,i);signal(&meter,i+1);
        createEnemyLifeMeterTask_update(&meter);
        assert(meter.m30==1&&meter.m0==nullptr&&meter.m10[0]==123);
        assert(engine.m474_XPReceivedFromBattle==17&&engine.m478_dyneReceivedFromBattle==9);
        meter.m24=nullptr;position[0]=999;
        for(int tick=0;tick<100&&!meter.task.finished;++tick)createEnemyLifeMeterTask_update(&meter);
        assert(meter.task.finished&&meter.m2E_width==1&&meter.m10[0]==123);
        assert(engine.m474_XPReceivedFromBattle==17&&engine.m478_dyneReceivedFromBattle==9);
    }
    sEnemyLifeMeterTask meter{};s16 hp=75;meter.m24=&hp;
    meter.m1C=fixedPoint::fromInteger(100);meter.m2C_entryIndex=-1;
    signal(&meter,4);createEnemyLifeMeterTask_update(&meter);
    assert(meter.m20<0&&meter.m30==0&&meter.m2A==29&&!meter.task.finished);
    std::puts("Typed life-meter signals, native drain and one-time rewards PASS");
}

#include <cassert>
#include <cstdio>
#include <cstdint>
using s16=int16_t;
struct fixedPoint {
 double v=0; fixedPoint()=default; fixedPoint(double x):v(x){}
 operator double() const{return v;}
 fixedPoint& operator+=(fixedPoint x){v+=x.v;return *this;}
 static fixedPoint fromInteger(int x){return x;}
 int toInteger()const{return int(v);}
};
fixedPoint FP_Div(fixedPoint a,fixedPoint b){return double(a)/double(b);}
struct sVec3_FP{int x=0,y=0,z=0;};
struct Task{bool done=false;void markFinished(){done=true;}};
struct sEnemyLifeMeterTask{
 sVec3_FP* m0=nullptr; sVec3_FP m10; fixedPoint m1C,m20;
 s16* m24=nullptr; s16 m2A=-1,m2C_entryIndex=3,m2E_width=20;
 int m30=0; uint8_t m31=0; Task task;Task* getTask(){return &task;}
};
struct Debug{int mFlags[64]{};}debug;
struct Engine{int m474_XPReceivedFromBattle=0,m478_dyneReceivedFromBattle=0;}engine;
struct Overlay{Debug* m10_inBattleDebug=&debug;Engine* m4_battleEngine=&engine;}overlay;
struct Manager{Overlay* m10_battleOverlay=&overlay;}manager,*gBattleManager=&manager;
struct Common{int getSaturnPtr(int ea){return ea;}}common,*gCommonFile=&common;
int reads=0;
int readSaturnS16(int ea){++reads;assert(ea==0x201798+3*8||ea==0x20179A+3*8);return ea==0x201798+3*8?7:2;}
int performModulo2(int,int){assert(false);return 0;}
int randomNumber(){assert(false);return 0;}int performModulo(int,int){assert(false);return 0;}
void vdp2DebugPrintSetPosition(int,int){assert(false);}void vdp2PrintfSmallFont(const char*,...){assert(false);}
// PRODUCTION_NOTIFICATION
// PRODUCTION_UPDATE
int main(){
 s16 hp=100;sVec3_FP pos{1,2,3};sEnemyLifeMeterTask meter;
 meter.m0=&pos;meter.m24=&hp;meter.m1C=100;
 hp=80;meter.m31|=2;createEnemyLifeMeterTask_update(&meter);
 assert(reads==0&&engine.m474_XPReceivedFromBattle==0&&!meter.task.done);
 hp=0;meter.m31|=2;lagiBattleEnemyDefeated(&meter);
 createEnemyLifeMeterTask_update(&meter);
 assert(reads==2&&engine.m474_XPReceivedFromBattle==7&&engine.m478_dyneReceivedFromBattle==2);
 assert(!meter.m0&&meter.m10.x==1&&meter.m10.y==2&&meter.m10.z==3);
 for(int i=0;i<80&&!meter.task.done;++i)createEnemyLifeMeterTask_update(&meter);
 // Native narrow meters can finish shrinking before the HP interpolation drains.
 assert(meter.task.done&&double(meter.m1C)<80&&reads==2);
 sEnemyLifeMeterTask noRecord;noRecord.m24=&hp;noRecord.m1C=10;noRecord.m2C_entryIndex=-1;
 lagiBattleEnemyDefeated(&noRecord);createEnemyLifeMeterTask_update(&noRecord);assert(reads==2);
 lagiBattleEnemyDefeated<sEnemyLifeMeterTask>(nullptr);
 puts("PASS production meter: damage, authored reward lookup once, position detach, death retirement, no-record actor");
}

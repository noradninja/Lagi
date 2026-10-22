#include <cassert>
#include <cstdint>
#include <cstdio>
using s32=int32_t;
enum eBattleRating {narrow,close,good,great,excellent};
struct sBattleResultScreen {};
struct sSaturnPtr {
    int offset;
    bool isNull()const{return offset==0;}
    sSaturnPtr operator+(int n)const{return {offset+n};}
};
static int overlayIndex,formationIndex;
static bool noTable;
static int16_t thresholds[8]={10,20,30,40,100,200,300,400};
struct Generic {sSaturnPtr getSaturnPtr(int address){assert(address==0x060AB010);return {100};}} generic;
static auto* g_BTL_GenericData=&generic;
static sSaturnPtr readSaturnEA(sSaturnPtr ptr){assert(ptr.offset==100+overlayIndex*4);return {noTable?0:1000};}
static int16_t readSaturnS16(sSaturnPtr ptr){int index=(ptr.offset-1000-formationIndex*16)/2;assert(index>=0&&index<8);return thresholds[index];}
static s32 performDivision(s32 divisor,s32 dividend){return dividend/divisor;}
struct Engine{s32 m190[2];} engine;
struct Overlay{Engine* m4_battleEngine=&engine;} overlay;
struct Manager{int m2_currentBattleOverlayId=0,m6_subBattleId=0;Overlay* m10_battleOverlay=&overlay;} manager;
static auto* gBattleManager=&manager;
#include "../src/integration/lagi_battle_rating.inc"
int main(){
    const int result[9]={4,3,2,1,1,1,1,0,0};
    for(int ov=0;ov<17;++ov)for(int sub=0;sub<10;++sub){
        overlayIndex=manager.m2_currentBattleOverlayId=ov;
        formationIndex=manager.m6_subBattleId=sub;
        for(int a=0;a<5;++a)for(int b=0;b<5;++b){
            engine.m190[0]=a*10*30;engine.m190[1]=b*100;
            assert(battleResultScreen_computeBattleRating(nullptr)==result[a+b]);
        }
    }
    engine.m190[0]=299;engine.m190[1]=99;
    assert(battleResultScreen_computeBattleRating(nullptr)==excellent);
    engine.m190[0]=300;
    assert(battleResultScreen_computeBattleRating(nullptr)==great);
    noTable=true;
    assert(battleResultScreen_computeBattleRating(nullptr)==narrow);
    puts("PASS native rank bands, equality boundaries, update division, overlay/subbattle indexing and absent table");
}

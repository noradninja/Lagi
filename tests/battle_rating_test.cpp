#include <cassert>
#include <cstdint>
#include <cstdio>
#include <array>
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
struct Engine{s32 m190[2];std::array<std::int16_t,10> m484;Engine(){m484.fill(-1);}} engine;
struct Overlay{Engine* m4_battleEngine=&engine;} overlay;
struct Manager{int m2_currentBattleOverlayId=99,m6_subBattleId=99,m4_previousBattleOverlayId=0,m8_previousSubBattleId=0;Overlay* m10_battleOverlay=&overlay;} manager;
static auto* gBattleManager=&manager;
std::array<std::uint8_t,0x104> battleResults{};
template<class... T>void PDS_Log(const char*,T...){}
#include "../src/integration/lagi_battle_rating.inc"
int main(){
    const int result[9]={4,3,2,1,1,1,1,0,0};
    for(int ov=0;ov<17;++ov)for(int sub=0;sub<10;++sub){
        overlayIndex=manager.m4_previousBattleOverlayId=ov;
        formationIndex=manager.m8_previousSubBattleId=sub;
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
    noTable=false;engine.m190[0]=0;engine.m190[1]=0;
    engine.m484={0,129,5,-1,8,-1,-1,-1,-1,-1};
    battleResults[0]=254;battleResults[1]=3;battleResults[258]=255;battleResults[259]=4;
    assert(battleResultScreen_computeBattleRating(nullptr)==excellent);
    assert(battleResults[0]==255 && battleResults[1]==4 && battleResults[258]==255 && battleResults[259]==4);
    assert(battleResults[10]==1 && battleResults[11]==4 && battleResults[16]==0);
    noTable=true;assert(battleResultScreen_computeBattleRating(nullptr)==narrow);
    assert(battleResults[0]==255 && battleResults[1]==4 && battleResults[10]==2 && battleResults[11]==4);
    battleResults.fill(0);for(unsigned i=0;i<10;++i)engine.m484[i]=static_cast<std::int16_t>(i);
    for(int rank=0;rank<5;++rank)lagiRecordBattleResult(rank);
    for(unsigned i=0;i<10;++i)assert(battleResults[i*2]==5 && battleResults[i*2+1]==4);
    for(unsigned i=0;i<300;++i)lagiRecordBattleResult(0);
    for(unsigned i=0;i<10;++i)assert(battleResults[i*2]==255 && battleResults[i*2+1]==4);
    puts("PASS native rank bands/indexing and result tail: ten slots, first-negative stop, upper record ID, count saturation, best-rank preservation and absent-table bookkeeping");
}

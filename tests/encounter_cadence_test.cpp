#include <array>
#include <vector>
#include <functional>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <algorithm>
using u8=std::uint8_t;using s8=std::int8_t;using s16=std::int16_t;using s32=std::int32_t;using u32=std::uint32_t;
static std::array<u8,2048> memory{};
struct sSaturnPtr {
    u32 m_offset=0;bool isNull()const{return !m_offset;}
    static sSaturnPtr getNull(){return {};}
    sSaturnPtr operator+(unsigned n)const{return {m_offset+n};}
    sSaturnPtr& operator+=(unsigned n){m_offset+=n;return *this;}
};
u8* getSaturnPtr(sSaturnPtr p){assert(p.m_offset<memory.size());return memory.data()+p.m_offset;}
s8 readSaturnS8(sSaturnPtr p){return s8(*getSaturnPtr(p));}
s16 readSaturnS16(sSaturnPtr p){return s16((u32(*getSaturnPtr(p))<<8)|*getSaturnPtr(p+1));}
s32 readSaturnS32(sSaturnPtr p){return s32((u32(std::uint16_t(readSaturnS16(p)))<<16)|std::uint16_t(readSaturnS16(p+2)));}
sSaturnPtr readSaturnEA(sSaturnPtr p){return {u32(readSaturnS32(p))};}
void put(unsigned at,u32 value,unsigned bytes){for(unsigned i=0;i<bytes;++i)memory[at+i]=u8(value>>((bytes-i-1)*8));}
struct s_dragonTaskWorkArea {std::array<s32,3> m8_pos{};int m235_dragonSpeedIndex=1;} dragon;
struct Subfield {s_dragonTaskWorkArea* m338_pDragonTask=&dragon;} subfield;
struct Field {Subfield* m8_pSubFieldData=&subfield;int m2E_currentSubFieldIndex=0,m34_radarDangerLevel=0;u32 m28_status=0;} field;
Field* fieldTaskPtr=&field;
struct sEncouterTask {sSaturnPtr m18{0x100},m1C{};int mD=-1,mE=0,mF=0,m10=0,m14=0,m16=0;} encounter;
bool enableDebugTask=false;
struct {unsigned m10_palette=0;} vdp2PrintStatus;
void vdp2DebugPrintSetPosition(int,int){}
void vdp2DebugPrintNewLine(const char*){}
void vdp2PrintfSmallFont(const char*,...){}
void noEncounterData(){field.m34_radarDangerLevel=0;}
void lagiReportEncounterState(sEncouterTask*,sSaturnPtr,int){}
namespace lagi::platform::logging {template<class... T>void writef(const char*,T...) {}}
std::vector<u32> rolls;unsigned rollRead=0;
u32 randomNumber(){assert(rollRead<rolls.size());return rolls[rollRead++];}
u32 performModulo2(u32 divisor,u32 value){return value%divisor;}
#include "encounter_evaluate.inc"
int hasEncounterData=1,battleIndex=-1;
Field* getFieldTaskPtr(){return fieldTaskPtr;}
struct s_randomBattleWorkArea {int m4=0;static void randomBattleTaskDraw(s_randomBattleWorkArea*);};
#include "encounter_enable.inc"
void pollEncounter(){auto* pThis=&encounter;
#include "encounter_gate.inc"
}
struct s_task;
struct Work {
    int updates=0,draws=0;std::function<void()> hook;
    void Update(){++updates;if(hook)hook();}void Draw(){++draws;}void Delete(){assert(false);}
};
constexpr unsigned TASK_FLAGS_DELETING=4;
struct s_task {
    s_task* m0_pNextTask=nullptr; s_task* m4_pSubTask=nullptr;
    unsigned m14_flags=0;Work work;
    bool isFinished()const{return m14_flags&1;}bool isPaused()const{return m14_flags&2;}
    bool isDeleting()const{return m14_flags&TASK_FLAGS_DELETING;}
    void markFinished(){m14_flags|=1;}Work* getWorkArea(){return &work;}
};
s_task* taskListHead=nullptr;int numActiveTask=0;
std::array<u8,3> pauseEngine{};
#include "encounter_scheduler.inc"
s_task root,poll,peer;
void setup(s8 chance=8,s8 cap=0){
    memory.fill(0);encounter={};field={};dragon={};hasEncounterData=1;battleIndex=-1;
    rolls.clear();rollRead=0;pauseEngine={};root={};poll={};peer={};
    root.m4_pSubTask=&poll;poll.m0_pNextTask=&peer;poll.work.hook=pollEncounter;
    taskListHead=&root;
    put(0x100,0x200,4);put(0x104,5,2);memory[0x106]=3;memory[0x107]=2;
    put(0x200,0x300,4);put(0x208,0x300,4);
    memory[0x300]=u8(chance);memory[0x301]=1;memory[0x302]=u8(cap);
    put(0x304,u32(-10),2);put(0x306,u32(-10),2);put(0x308,10,2);put(0x30a,10,2);
    put(0x30c,0x500,4);put(0x31c,0xffffffffu,4);
    memory[0x500]=5;memory[0x501]=50;memory[0x502]=7;memory[0x503]=50;
}
int main(){
    setup();rolls={99,99,49};
    runTasks();assert(encounter.m10==2 && rollRead==0);
    runTasks();assert(encounter.m10==1 && rollRead==0);
    runTasks();assert(encounter.m10==5 && encounter.mE==1 && rollRead==1 && battleIndex==-1);
    for(unsigned i=0;i<4;++i)runTasks();
    assert(encounter.m10==1 && rollRead==1);
    runTasks();assert(battleIndex==5 && encounter.mE==2 && rollRead==3);
    assert(root.work.updates==8 && poll.work.updates==8 && peer.work.updates==8);
    assert(poll.work.draws==8); // one Update/Draw per traversal, no catch-up multiplier
    setup();rolls={7,50};for(unsigned i=0;i<3;++i)runTasks();
    assert(battleIndex==7 && rollRead==2); // native roll < 8, then selection at 50
    setup();rolls={8};for(unsigned i=0;i<3;++i)runTasks();
    assert(battleIndex==-1 && encounter.mE==1 && encounter.m10==5 && rollRead==1);
    setup();dragon.m235_dragonSpeedIndex=0;
    for(unsigned i=0;i<500;++i)runTasks();
    assert(encounter.m10==3 && rollRead==0 && field.m34_radarDangerLevel==1);
    setup();pauseEngine[0]=1;
    for(unsigned i=0;i<500;++i)runTasks();
    assert(poll.work.updates==0 && poll.work.draws==500 && encounter.mD==-1 && rollRead==0);
    setup();poll.m14_flags=2;
    for(unsigned i=0;i<500;++i)runTasks();
    assert(poll.work.updates==0 && poll.work.draws==0 && peer.work.updates==500 && rollRead==0);
    setup();root.m14_flags=2;
    for(unsigned i=0;i<500;++i)runTasks();
    assert(root.work.updates==0 && poll.work.updates==0 && peer.work.updates==0 && rollRead==0);
    setup();hasEncounterData=0;encounter.m10=77;runTasks();
    assert(encounter.m10==77 && encounter.mD==-1 && rollRead==0 && battleIndex==-1);
    setup(0);for(unsigned i=0;i<100;++i)runTasks();assert(encounter.m10==3 && rollRead==0);
    setup();dragon.m8_pos[0]=11<<12;runTasks();assert(encounter.m10==3 && rollRead==0 && field.m34_radarDangerLevel==0);
    for(int x:{-11,-10,10,11}) {
        setup();dragon.m8_pos[0]=x*4096;runTasks();
        const bool inside=x>=-10 && x<=10;
        assert(encounter.m10==(inside?2:3) && rollRead==0);
    }
    setup(-1,2);rolls={0,50,0};
    runTasks();assert(battleIndex==5 && memory[0x303]==1 && rollRead==1);
    battleIndex=-1;runTasks();assert(battleIndex==7 && memory[0x303]==2 && rollRead==2);
    battleIndex=-1;runTasks();assert(battleIndex==-1 && memory[0x303]==2 && rollRead==2);
    field.m2E_currentSubFieldIndex=2;runTasks();
    assert(battleIndex==5 && memory[0x303]==1 && encounter.mD==2 && rollRead==3);
    setup();field.m8_pSubFieldData=nullptr;runTasks();assert(encounter.mD==-1 && encounter.m1C.isNull() && rollRead==0);
    setup();field.m2E_currentSubFieldIndex=1;runTasks();assert(encounter.mD==-1 && encounter.m1C.isNull() && rollRead==0);
    for(int policy:{0,1,2,3})for(u32 status:{0u,1u,0x10000u,0x10001u}) {
        s_randomBattleWorkArea area{policy};field.m28_status=status;
        s_randomBattleWorkArea::randomBattleTaskDraw(&area);
        const bool expected=(policy==2)||(status==0)||((policy!=0)&&((status&0xffff)==0));
        assert(hasEncounterData==int(expected) && isBattleEnabled()==(status==0));
    }
    puts("PASS actual Vita scheduler plus generated encounter gates/evaluator: one update per traversal, pauses, movement, countdown/reset, failure cap, row quotas, weighted boundary and subfield reset; not whole Saturn timing proof");
}

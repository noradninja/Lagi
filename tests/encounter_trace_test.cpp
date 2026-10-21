#include <array>
#include <cassert>
#include <cstdint>
#include <cstdarg>
#include <cstdio>
#include <string>
using s32=int32_t;using u32=uint32_t;
struct sSaturnPtr{u32 m_offset;bool isNull()const{return m_offset==0;}sSaturnPtr operator+(unsigned n)const{return {m_offset+n};}};
int readSaturnS8(sSaturnPtr p){assert(p.m_offset==100||p.m_offset==101);return p.m_offset==100?8:1;}
struct Dragon{int m235_dragonSpeedIndex=1;};Dragon dragon;
struct Sub{Dragon* m338_pDragonTask=&dragon;};Sub sub;
struct Field{Sub* m8_pSubFieldData=&sub;int m2C_currentFieldIndex=2,m2E_currentSubFieldIndex=0;u32 m28_status=0;int m34_radarDangerLevel=1;}field;
Field* fieldTaskPtr=&field;
struct State{int consumables[256]{};}mainGameState;
int hasEncounterData=1;
struct sEncouterTask{sSaturnPtr m18{0x207634};int m10=40,m14=90,m16=120,mE=0,mF=10;};
int logs=0;std::string last;
#define PDS_Log(...) // Match the Vita no-op: the observer must not use it.
namespace lagi::platform::logging {
void writef(const char* format,...){char line[1024];va_list args;va_start(args,format);vsnprintf(line,sizeof line,format,args);va_end(args);last=line;++logs;}
}
#include "lagi_encounter_trace.inc"
int main(){
 sEncouterTask task;
 lagiReportEncounterState(&task,{100},3);
 assert(logs==1&&last.find("rate=8 rowDanger=1 moving=1 shard=0 reason=3")!=std::string::npos);
 for(int i=0;i<1000;++i){--task.m10;lagiReportEncounterState(&task,{100},3);}
 assert(logs==1); // no per-frame timer logging
 assert(task.m10==-960&&task.mE==0&&field.m34_radarDangerLevel==1);
 mainGameState.consumables[56]=1;lagiReportEncounterState(&task,{100},3);assert(logs==2);
 field.m28_status=8;lagiReportEncounterState(&task,{100},3);assert(logs==3);
 dragon.m235_dragonSpeedIndex=0;lagiReportEncounterState(&task,{100},3);assert(logs==4);
 field.m8_pSubFieldData=nullptr;lagiReportEncounterState(&task,{0},1);
 assert(logs==5&&last.find("row=00000000 rate=0 rowDanger=-1 moving=-1")!=std::string::npos);
 lagiReportEncounterState(&task,{0},1);assert(logs==5);
 hasEncounterData=0;lagiReportEncounterState(&task,{0},0);assert(logs==6);
 puts("PASS: production encounter trace deduplication, gate changes, missing field/row safety and unchanged counters/danger.");
}

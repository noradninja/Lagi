#include <cstdio>
#include <cassert>
#include <cstdint>
#include <vector>
#include <utility>

struct Task { bool done=false; void markFinished(){done=true;} };
using p_workArea=void*;
template<class T> struct s_workAreaTemplate {
 struct TypedTaskDefinition { void* init; void(*update)(T*); void* draw; void(*destroy)(T*); };
 Task task;
 Task* getTask(){return &task;}
};
void* lastTask = nullptr;
template<class T> T* createSubTask(p_workArea, const typename T::TypedTaskDefinition*) {
 auto* task = new T{}; lastTask = task; return task;
}
struct s_battleEngine { int m3A8_overlayBattledata=0x100; int m3B0_subBattleId=1; };
int reads=0, draws=0, clears=0, writes=0, stops=0;
int expectedSub=1, expectedText=4;
const char* expectedPCM="B_A5_00.PCM";
unsigned queued=0;
int readSaturnEA(int offset) {
 if (reads++==0) {assert(offset==0x100+expectedSub*0x20+0xC);return 0x200;}
 assert(offset==0x200+expectedText*4);return 0x300;
}
#include <string>
std::string readSaturnString(int offset) {assert(offset==0x300);return "fixture";}
void setupVDP2StringRendering(int x,int y,int w,int h) { assert(x==3&&y==25&&w==38&&h==4); }
void clearVdp2TextArea(){++clears;}
void VDP2DrawString(const char*){++draws;}
#include "../src/integration/lagi_nanyd_intro.inc"
int lagi::disc::selected_disc(){return 1;}
bool lagi::disc::read_file(const char* name, std::vector<std::uint8_t>& out) {
 assert(std::string(name)==expectedPCM); out={1,2}; return true;
}
void lagi::platform::logging::writef(const char*, ...) {}
int lagiSubmitCutscenePCM(const void*, const std::uint8_t* d,std::size_t n,unsigned rate,unsigned ch,unsigned bits,const std::uint8_t*,std::size_t){
 assert(n==2&&d[0]==1&&rate==22050&&ch==1&&bits==16);
 if(writes++==0)return -1;
 queued=1;
 return 0;
}
unsigned lagiCutscenePCMQueued(const void*){return queued;}
void lagiStopCutscenePCM(const void*){++stops;}
int main(){
 for (int opening=0;opening<2;++opening) {
 reads=draws=clears=writes=stops=0;queued=0;
 expectedSub=opening?4:1;expectedText=opening?9:4;
 expectedPCM=opening?"B_A5_03.PCM":"B_A5_00.PCM";
 s_battleEngine engine;engine.m3B0_subBattleId=expectedSub;
 if(opening)lagiStartBattleDialogue(nullptr,&engine,expectedText,expectedPCM);
 else lagiStartNanydDialogue(nullptr,&engine);
 assert(reads==2 && draws==1 && clears==1);
 auto& task=*static_cast<NanydDialogue*>(lastTask);
 task.Update(&task);
 assert(task.pcm && !task.task.done);
 task.Update(&task);
 assert(!task.pcm && !task.task.done && queued==1);
 task.Update(&task);
 assert(!task.task.done);
 queued=0;
 task.Update(&task);
 assert(task.task.done);
 task.Delete(&task);
 assert(clears==2 && stops==1);
 delete &task;
 }
 puts("PASS both production battle openings: authored subtitle/PCM selection, submit retry, drain and cleanup");
}

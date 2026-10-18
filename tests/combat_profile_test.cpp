#include <cassert>
#include <cstdio>
#include <cstdint>
unsigned clockReads=0,runs=0,publications=0;
unsigned published=0;
uint64_t sceKernelGetProcessTimeWide(){return (++clockReads==1)?1000:2345;}
void runTasks(){assert(clockReads==1&&publications==0);++runs;}
namespace lagi::platform::renderer{
void presentation_profile_tasks_us(unsigned time){assert(runs==1&&clockReads==2);published=time;++publications;}
}
void tick(){
#include "task_profile.inc"
}
int main(){tick();assert(runs==1&&clockReads==2&&publications==1&&published==1345);std::puts("Actual task-profile boundary and publication order PASS");}

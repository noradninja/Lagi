#include <cassert>
#include <cstdint>
#include "../src/integration/lagi_a5_scatter_lifecycle.h"

struct Task { bool done=false; bool isFinished()const{return done;} void markFinished(){done=true;} };
struct Group { int mF_deadCount=0; };
struct Formation { Group mE0_entityGroup; };
struct Enemy {
 int m28_state=0; std::int16_t m24_counter=0; Formation* m8_formation=nullptr;
 Task task; Task* getTask(){return &task;}
};
int main() {
 Formation f; Enemy e; e.m8_formation=&f;
 assert(!lagiA5ScatterRetireIfComplete(&e));
 e.m28_state=6;
 for(int i=0;i<91;++i){assert(!lagiA5ScatterRetireIfComplete(&e));assert(f.mE0_entityGroup.mF_deadCount==0);}
 assert(lagiA5ScatterRetireIfComplete(&e));
 assert(e.task.done && f.mE0_entityGroup.mF_deadCount==1 && e.m24_counter==0);
 assert(!lagiA5ScatterRetireIfComplete(&e));
 assert(f.mE0_entityGroup.mF_deadCount==1);
}

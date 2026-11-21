#include <cassert>
#include <cstdint>
#include <cstdio>
#include <algorithm>
using s16=int16_t;
struct Task{bool done=false;void markFinished(){done=true;}};
struct Number{int v=0;Number& operator+=(int n){v+=n;return *this;}Number& operator-=(int n){v-=n;return *this;}operator int()const{return v;}};
struct Vec{Number m0_X,m4_Y,m8_Z;};
struct sLagiA5StoryWorm{Task task;Vec position,rotation;int speed=0x1AAA;Task* getTask(){return &task;}};
struct sSaturnPtr{int offset=0;sSaturnPtr& operator+=(int n){offset+=n;return *this;}};
template<class T>struct s_workAreaTemplate{Task task;Task* getTask(){return &task;}};
struct State{int sets=0,bit=0;void setBit566(int n){++sets;bit=n;}}mainGameState;
struct Field{uint32_t m28_status=0x40;}field;
Field* getFieldTaskPtr(){return &field;}
s16 command=0;int reads=0;
s16 readSaturnS16(sSaturnPtr){++reads;return command;}
int sounds=0;void playSystemSoundEffect(int n){assert(n==0x6C);++sounds;}
// PRODUCTION_TRACKER
int main(){
 for(int mask=0;mask<4096;++mask){
  sLagiA5StoryWorm actor;sLagiA5WormTracker t;t.actor=&actor;t.threshold=100;t.bit=0x74B;
  actor.position.m8_Z.v=99;actor.position.m4_Y.v=10000;command=mask;
  auto sets=mainGameState.sets;sLagiA5WormTracker::Update(&t);
  assert(mainGameState.sets==sets); // Y cannot satisfy the Z threshold.
  assert(int(actor.rotation.m0_X)==((mask&1)?0x888888:0)-((mask&2)?0x888888:0));
  assert(int(actor.rotation.m4_Y)==((mask&4)?0x888888:0)-((mask&8)?0x888888:0));
  assert(int(actor.rotation.m8_Z)==((mask&0x800)?0x888888:0)-((mask&0x400)?0x888888:0));
  assert(actor.speed==0x1AAA+((mask&0x10)?0x111:0)-((mask&0x20)?0x111:0));
  assert(t.commands.offset==2&&(field.m28_status&0x8000));
  command=0;actor.position.m8_Z.v=100;sLagiA5WormTracker::Update(&t);
  assert(mainGameState.sets==sets+1&&mainGameState.bit==0x74B&&t.state==2);
  sLagiA5WormTracker::Update(&t);assert(mainGameState.sets==sets+1);
  command=-1;const auto offset=t.commands.offset;sLagiA5WormTracker::Update(&t);
  assert(actor.task.done&&t.task.done&&mainGameState.sets==sets+2);
  assert(t.commands.offset==offset&&field.m28_status==0x40);
 }
 sLagiA5StoryWorm actor;sLagiA5WormTracker t;t.actor=&actor;t.threshold=100;t.bit=0x74C;
 actor.speed=0;command=0x20;int before=sounds;
 for(int i=0;i<46;++i)sLagiA5WormTracker::Update(&t);
 assert(actor.speed==0&&sounds-before==3&&t.commands.offset==92);
 puts("PASS production worm tracker: 4096 control masks, Z threshold once, script-end cleanup, preserved field flags, speed floor and authored sound frames");
}

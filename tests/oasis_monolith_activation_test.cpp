
#include <cassert>
#include <cstdint>
#include <array>
#include <cstdio>
using s32=int32_t;using u32=uint32_t;using p_workArea=void*;
struct fixedPoint{int v=0;fixedPoint(int n=0):v(n){}int asS32()const{return v;}fixedPoint& operator-=(int n){v-=n;return *this;}fixedPoint& operator+=(fixedPoint n){v+=n.v;return *this;}};
struct sVec3_FP:std::array<fixedPoint,3>{};
struct s_fileBundle{uint8_t raw[32]{};uint8_t* getRawBuffer(){return raw;}};
unsigned READ_BE_U32(uint8_t*){return 0x1234;}
struct sDebrisScatterParams{fixedPoint m8_spread,m28_rotVelY,m30_angleOffset,m38;unsigned m44_soundEffect;unsigned model,pose;sVec3_FP* m10_pPosition; sVec3_FP* m14_pRotation;s_fileBundle* m_pBundle;};
struct s_3dModel{};struct sFieldModelRenderContext{};
struct sA5SkyEntity{struct{s_fileBundle* m0_mainMemoryBundle;}m0_memoryArea;sVec3_FP mE8_position,mF8_rotation;int m104=0,m108=0,m10C=0,m114=0,mF4=0;bool m11C_flag=false;void(*m_UpdateMethod)(sA5SkyEntity*)=nullptr;s_3dModel m98_model2;void* lagiDebris=nullptr;void(*m_DrawMethod)(sA5SkyEntity*)=nullptr;struct{void(*m18_drawFunction)(void*);}m48_model1;};
bool culled=false;int draws=0,pushes=0,pops=0,sounds=0,found=0;unsigned configs=0;bool reversed=false;sDebrisScatterParams captured;
void pushCurrentMatrix(){++pushes;}void popMatrix(){++pops;}void translateCurrentMatrix(sVec3_FP*){}void rotateCurrentMatrixZYX(sVec3_FP*){}
bool checkPositionVisibilityAgainstFarPlane(sVec3_FP*){return culled;}
int findSound(int n){assert(n==0x70);return found;}
void enqueuePlaySoundEffect(int id,int command,int value,int arg){assert(id==0x70&&command==5&&!value&&arg==2);++sounds;}
void initDebrisScatterConfig(sDebrisScatterParams* c,unsigned model,unsigned pose){*c={};c->model=model;c->pose=pose;}
int readMaxScalarFromBundleTree(uint8_t*,unsigned root){assert(root==0x1234);return 0x4567;}
void* createDebrisScatterTask(sA5SkyEntity* a,sDebrisScatterParams* c,bool reverse){++configs;captured=*c;reversed=reverse;return a;}
// SETTLED

struct Game{uint8_t bitField[256]{};}mainGameState;
struct File{u32 getSaturnPtr(u32 a){return a;}}file;auto* gFLD_A5=&file;
unsigned rng=1,randomCalls=0,bursts=0,smallRings=0,rings=0,finals=0,cameras=0,scripts=0,pcm=0,systemSound=0;
u32 randomNumber(){++randomCalls;rng=rng*1664525u+1013904223u;return rng;}
void lagiOasisMonolithBurst(sA5SkyEntity*){++bursts;}
void lagiOasisMonolithRing(sA5SkyEntity*,bool small){if(small)++smallRings;else ++rings;}
void lagiOasisMonolithFinalRings(sA5SkyEntity*){++finals;}
void startFieldScript(int n,int bit){assert(n==5&&bit==0x5BA);++scripts;}
void playPCM(sA5SkyEntity*,int id){assert(id==0x70);++pcm;}
void playSystemSoundEffect(int id){assert(id==0x73);++systemSound;}
void a5CutsceneCameraInit(sA5SkyEntity* a,u32 script,u32 callback,sVec3_FP* position,int flags){assert(script==0x608C8AC&&callback==0x605B58A&&position==&a->mE8_position&&!flags);++cameras;}
// ACTIVATION
int main(){
 for(unsigned seed=0;seed<1024;++seed){
  s_fileBundle bundle;sA5SkyEntity actor;actor.m0_memoryArea.m0_mainMemoryBundle=&bundle;rng=seed;randomCalls=bursts=smallRings=rings=finals=cameras=scripts=pcm=systemSound=0;mainGameState={};mainGameState.bitField[0x96]=0x40;
  assert(!(mainGameState.bitField[0x95]&2));
  lagiOasisMonolithActivate(&actor,nullptr);assert(actor.m_UpdateMethod==lagiOasisMonolithActivationUpdate&&scripts==1&&cameras==1&&bursts==1&&mainGameState.bitField[0x96]==0x60&&actor.m114==0);
  for(int tick=1;tick<=120;++tick){actor.m_UpdateMethod(&actor);assert(bool(mainGameState.bitField[0x95]&2)==(tick==120));if(tick<120)assert(!finals&&!pcm&&!systemSound&&actor.m114==tick);}
  assert(finals==1&&pcm==1&&systemSound==1&&actor.m114==240&&actor.m_UpdateMethod==lagiOasisMonolithSettledUpdate&&actor.m_DrawMethod==lagiOasisMonolithSettledDraw);
  assert(smallRings==6+24&&rings==3+37); // ticks 16..56 / 60..83 / 84..120
  for(int tick=0;tick<240;++tick)actor.m_UpdateMethod(&actor);assert(actor.m114==0&&finals==1&&pcm==1);
 }
 puts("PASS production monolith destruction callback, exact script/camera, 120-update completion, effect schedule, saved flags and settled handoff across 1024 seeds");
}

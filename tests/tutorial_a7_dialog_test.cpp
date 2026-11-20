#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <string>
using u8=std::uint8_t;using s8=std::int8_t;using u32=std::uint32_t;using s32=std::int32_t;
struct Task {bool paused=false,finished=false;void markPaused(){paused=true;}void clearPaused(){paused=false;}void markFinished(){finished=true;}};
struct Work {Task task;Task* getTask(){return &task;}};
using p_workArea=Work*;
template<class T>struct s_workAreaTemplate:Work {
 struct TypedTaskDefinition {void(*init)(T*);void(*update)(T*);void(*draw)(T*);void(*destroy)(T*);};
};
static Work* lastCreated=nullptr;static bool allocationFails=false;
template<class T>T* createSiblingTask(p_workArea,const typename T::TypedTaskDefinition*) {
 if(allocationFails)return nullptr;auto* task=new T{};lastCreated=task;return task;
}
static constexpr unsigned Base=0x6054000;
static std::array<u8,0x40000> memory;
struct sSaturnPtr {s32 m_offset;void* m_file; sSaturnPtr operator+(unsigned n)const{return {s32(unsigned(m_offset)+n),m_file};}};
struct Overlay {sSaturnPtr getSaturnPtr(u32 a){assert(a>=Base && a<Base+memory.size());return {s32(a),this};}}overlay;
static Overlay* gFLD_A7=&overlay;
static u8 readSaturnU8(sSaturnPtr p){return memory.at(unsigned(p.m_offset)-Base);}
static u32 readSaturnU32(sSaturnPtr p){u32 v=0;for(unsigned i=0;i<4;++i)v=(v<<8)|readSaturnU8(p+i);return v;}
static std::string readSaturnString(sSaturnPtr p){return std::to_string(p.m_offset);}
static void word(unsigned a,unsigned v){for(unsigned i=0;i<4;++i)memory.at(a-Base+i)=u8(v>>(24-i*8));}
struct Dragon {int m1C_dragonArchetype=0;}dragon;static Dragon* gDragonState=&dragon;
struct Input {unsigned m8_newButtonDown=0;};
struct {struct {struct {Input m0_current;}m0_inputDevices[1];}m4514;struct {int m0_menuId=0;}m40AC;}graphicEngineStatus;
struct {int m0=7,m1C=0;}vdp2StringContext;
static unsigned boxes=0,clears=0,textClears=0,names=0,sounds=0;
static void setupVDP2StringRendering(int a,int b,int c,int d){assert(a==6 && b==5 && c==32 && d==14);}
static void clearVdp2TextArea(){++textClears;}
static void drawObjectName(const char* p){assert(p && *p);++names;}
static void drawBlueBox(int a,int b,int c,int d,int e){assert(a==3&&b==4&&c==40&&d==16&&e==0x1000);++boxes;}
static void clearBlueBox(int a,int b,int c,int d){assert(a==3&&b==4&&c==40&&d==16);++clears;}
static void playSystemSoundEffect(int sound){assert(sound==6);++sounds;}
#include "native_a7_dialog.inc"
int main() {
 for(unsigned alignment=0;alignment<4;++alignment) {
  const unsigned cursor=Base+0x100+alignment,aligned=(cursor+4)&~3u;
  memory[cursor-Base]=0x15;word(aligned,Base+0x200);
  sA7DialogChoiceTask task{};task.m4_dialogScriptEA=cursor;task.mD_state=99;
  assert(a7DialogChoiceScriptStep_0605eab0(&task)==Base+0x200);
  assert(task.m4_dialogScriptEA==aligned+4 && task.mD_state==0);
 }
 for(unsigned opcode=0;opcode<256;++opcode)if(opcode!=0x15) {
  memory[0x100]=u8(opcode);sA7DialogChoiceTask task{};task.m4_dialogScriptEA=Base+0x100;task.mD_state=99;
  assert(!a7DialogChoiceScriptStep_0605eab0(&task));
  assert(task.m4_dialogScriptEA==Base+0x101 && task.mD_state==(opcode==0x18?10:0));
 }
 for(unsigned topic=0;topic<5;++topic) {
  const unsigned script=Base+0x300+topic*16;memory[script-Base]=0x15;
  word(script+4,Base+0x200);memory[script+8-Base]=1;
  word(0x60861b0+(topic+0x21)*4,script);
  Work parent;a7DialogChoice_spawn_0605ea52(&parent,int(topic));
  auto* task=static_cast<sA7DialogChoiceTask*>(lastCreated);
  assert(parent.task.paused && task->m0_parent==&parent && task->m4_dialogScriptEA==script);
  const auto oldNames=names;vdp2StringContext.m0=7;
  a7DialogChoice_Draw_0605e9d6(task);
  assert(names==oldNames+1 && task->mC_drawSubState==2 && vdp2StringContext.m0==7);
  graphicEngineStatus.m4514.m0_inputDevices[0].m0_current.m8_newButtonDown=0;
  a7DialogChoice_Update_0605e97c(task);assert(task->m4_dialogScriptEA==script+8);
  graphicEngineStatus.m4514.m0_inputDevices[0].m0_current.m8_newButtonDown=2;
  a7DialogChoice_Update_0605e97c(task);assert(task->mC_drawSubState==1 && task->m8_drawnNameEA==0);
  a7DialogChoice_Draw_0605e9d6(task);assert(!parent.task.paused && task->task.finished);
  a7DialogChoice_Delete_0605ea46(task);delete task;
 }
 assert(clears==5 && sounds==5);
 for(unsigned archetype=0;archetype<8;++archetype) {
  dragon.m1C_dragonArchetype=int(archetype);
  word(0x60861b0+(archetype+0x26)*4,Base+0x500);
  memory[0x500]=0x15;word(Base+0x504,Base+0x200+archetype);
  sA7DialogChoiceTask task{};task.mD_state=10;
  a7DialogChoice_Update_0605e97c(&task);
  assert(task.m8_drawnNameEA==Base+0x200+archetype && task.mD_state==0 && task.mC_drawSubState==1);
 }
 Work parent;allocationFails=true;a7DialogChoice_spawn_0605ea52(&parent,0);assert(!parent.task.paused);
 puts("PASS production A7 dialogue: all opcodes/alignment, five topics, confirm/parent lifecycle, archetype reload, text restoration and allocation failure; mocked services, device pending");
}

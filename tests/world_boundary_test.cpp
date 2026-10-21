#include <cassert>
#include <vector>
#include <cstdint>
#include <cstring>
#include <cstdio>
using p_workArea=void*;using s32=int;
template<class T> struct s_workAreaTemplate {struct TypedTaskDefinition {void(*init)(T*);void *update,*draw,*destroy;};};
template<class T> void* createSubTask(void*){auto* task=new T;T::getTypedTaskDefinition()->init(task);return task;}
struct {int m4_gameStatus=0x4F,m0_gameMode=6;} gGameStatus;
bool available=true;int reads=0;
namespace lagi::disc {bool read_file(const char* name,std::vector<std::uint8_t>& out){assert(!std::strcmp(name,"WORLD.PRG"));++reads;if(available)out.assign(32,0);return available;}}
namespace lagi::platform::logging {template<class... T>void writef(const char*,T...) {}}
#include "production.inc"
int main(){int parent;
 for(bool present:{true,false}){available=present;auto* task=static_cast<LagiWorldBoundary*>(lagiLoadNativeWorldBoundary(&parent,0));
  assert(task&&static_cast<void*>(task)!=&parent);assert(task->program.size()==(present?32:0));
  assert(gGameStatus.m4_gameStatus==0x4F&&gGameStatus.m0_gameMode==6);delete task;}
 assert(reads==2);puts("PASS production WORLD boundary owns a real child/program, preserves native status, handles missing file without an intro fallback; map display remains unimplemented");
}

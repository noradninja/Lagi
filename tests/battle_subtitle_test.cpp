#include <cassert>
#include <string>
#include <cstdio>
using s16=short;
struct Ptr {int value=0;bool isNull()const{return value==0;}Ptr operator+(int x)const{return {value+x};}};
struct Task {bool finished=false;void markFinished(){finished=true;}};
using p_workArea=void*;
template<class T>struct s_workAreaTemplate {
 struct TypedTaskDefinition{void* init;void(*update)(T*);void* draw;void(*destroy)(T*);};
 Task task;Task* getTask(){return &task;}
};
void* allocated=nullptr;
template<class T>T* createSubTask(void*,const typename T::TypedTaskDefinition*){auto* t=new T;allocated=t;return t;}
struct Display{Ptr m0_texts{100};}display;
struct Overlay{Display* m14_textDisplay=&display;}overlay;
struct Manager{Overlay* m10_battleOverlay=&overlay;}manager,*gBattleManager=&manager;
struct {int m0=1;}vdp2StringContext;
int regions=0,clears=0,prints=0;
void setupVDP2StringRendering(int x,int y,int w,int h){assert(x==3&&y==25&&w==38&&h==4);++regions;}
void clearVdp2TextArea(){++clears;}
Ptr readSaturnEA(Ptr p){assert(p.value==164);return {900};}
std::string readSaturnString(Ptr p){assert(p.value==900);return "native speech";}
void VDP2DrawString(const char* p){assert(std::string(p)=="native speech");++prints;}
#include "production.inc"
int main(){
 lagiCreateBattleSubtitle(nullptr,16,60);
 auto* t=static_cast<LagiBattleSubtitle*>(allocated);
 assert(t&&prints==1&&regions==1&&vdp2StringContext.m0==0);
 for(int i=0;i<61;++i)LagiBattleSubtitle::Update(t);
 assert(!t->task.finished);
 LagiBattleSubtitle::Update(t);assert(t->task.finished);
 LagiBattleSubtitle::Delete(t);assert(clears==1&&regions==2);delete t;
 display.m0_texts={0};allocated=nullptr;lagiCreateBattleSubtitle(nullptr,16,60);assert(!allocated);
 puts("PASS native subtitle region, original string lookup, duration boundary and text-only cleanup; device pending");
}

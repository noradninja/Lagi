#include <cassert>
#include <cstdio>
struct {int m0_gameMode=3,m4_gameStatus=0x50;} gGameStatus;
struct Field {int m2C_currentFieldIndex=1,m2E_currentSubFieldIndex=0;} field;
Field* getFieldTaskPtr(){return &field;}
struct Specific {int m9C_A3_1_exits[3]={0,0,1};} specific;
Specific* getFieldSpecificData_A3(){return &specific;}
struct State {bool visited=false;bool getBit(int bit){assert(bit==86);return visited;}} mainGameState;
struct Channel {int color=0x4210,stopped=1;} ;
struct {Channel m0_fade0,m24_fade1;} g_fadeControls;
int calls=0;
void fadePalette(Channel* c,int from,int to,int steps){
 assert(from==0 && to==0 && steps==1);c->color=from;c->stopped=0;++calls;
}
#include "../src/integration/lagi_excavation_camera_cut.inc"
int main(){
 for(int exclusion=0;exclusion<6;++exclusion){
  switch(exclusion){
  case 0:gGameStatus.m0_gameMode=1;break;
  case 1:gGameStatus.m4_gameStatus=0x51;break;
  case 2:field.m2C_currentFieldIndex=0;break;
  case 3:field.m2E_currentSubFieldIndex=1;break;
  case 4:specific.m9C_A3_1_exits[2]=0;break;
  case 5:mainGameState.visited=true;break;
  }
  lagiCutCompletedExcavationCamera();assert(calls==0);
  gGameStatus={3,0x50};field={1,0};specific={{0,0,1}};mainGameState.visited=false;
 }
 lagiCutCompletedExcavationCamera();assert(calls==2);
 assert(g_fadeControls.m0_fade0.color==0 && g_fadeControls.m24_fade1.color==0);
 assert(!g_fadeControls.m0_fade0.stopped && !g_fadeControls.m24_fade1.stopped);
 puts("PASS camera-completion cut: exact route/exit/story gating, immediate dual black endpoints, native completion pending; hardware acceptance required");
}

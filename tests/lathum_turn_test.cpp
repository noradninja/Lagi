#include "lathum_rig_fixture.h"
struct s_battleEngineSub0{s16 m0=9;int m8=1;};
struct s_battleEngine{s_battleEngineSub0* m3CC;int m22C_dragonCurrentQuadrant=0;};
struct Input{u16 m6_buttonDown=0;};struct {struct{struct{Input m0_current;}m0_inputDevices[2];}m4514;}graphicEngineStatus;
bool playerTurn=false;int modeCalls=0,nameCalls=0,name=-1;enum{m7=7};
bool battleEngine_isPlayerTurnActive(){return playerTurn;}
void battleEngine_SetBattleMode(int value){assert(value==7);++modeCalls;}
void battleEngine_displayAttackName(int index,int duration,int flags){assert(modeCalls==1&&duration==30&&!flags);++nameCalls;name=index;}
#include "turn.inc"
int main(int argc,char**argv){
 assert(argc==2);original.bytes=load(argv[1]);
 LagiLathumRig rig;s_battleEngineSub0 ready;s_battleEngine engine{&ready};
 // Exhaust all three native part states, four quadrants, signed countdown
 // classes and controller2 override; controller1 must never select the variant.
 const unsigned first[4]={0,3,5,7},last[4]={3,5,7,9};
 for(unsigned states=0;states<19683;++states){
  unsigned digits=states;for(auto& group:rig.groups){group.state=digits%3;digits/=3;}
  for(unsigned q=0;q<4;++q)for(int counter:{-128,-1,0,1,2,127})for(u16 buttons:{u16(0),u16(4)}){
   ready={};ready.m8=123;rig.mode=0;rig.substate=6;rig.groupCounter=s8(counter);engine.m22C_dragonCurrentQuadrant=q;
   graphicEngineStatus.m4514.m0_inputDevices[1].m0_current.m6_buttonDown=buttons;
   graphicEngineStatus.m4514.m0_inputDevices[0].m0_current.m6_buttonDown=buttons^4;
   modeCalls=nameCalls=0;lagiSelectLathumTurn(rig,engine);
   bool damaged=false;for(unsigned i=first[q];i<last[q];++i)damaged|=rig.groups[i].state!=0;
   const int expected=counter==1?3:damaged?2:1;
   assert(rig.mode==expected&&!rig.substate&&modeCalls==1&&nameCalls==1&&!ready.m8&&!ready.m0);
   assert(rig.groupCounter==(counter>0?counter-1:counter));
   assert(name==(expected==3?(buttons&4?11:3):expected));
  }
 }
 for(bool active:{false,true})for(int turnReady:{0,1}){
  ready={};ready.m8=turnReady;playerTurn=active;rig.groupCounter=1;rig.mode=0;rig.substate=7;modeCalls=nameCalls=0;
  if(active||!turnReady){lagiSelectLathumTurn(rig,engine);assert(!modeCalls&&!nameCalls&&rig.groupCounter==1&&rig.substate==7&&ready.m0==9&&ready.m8==turnReady);}
 }
 puts("PASS Lathum turn selection: 944784 part/quadrant/countdown/input combinations and readiness guards");
}

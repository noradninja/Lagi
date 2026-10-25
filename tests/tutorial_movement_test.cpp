#include <cassert>
#include <cstdint>
using u8=uint8_t; using u16=uint16_t;
struct sTutorialControllerTask {u8 m1b_nextPhase=0,m1c_nextPhaseTarget=0,m1e_mes=0;int m10_timer=0;};
struct Timer {int m4=2;} timer;
struct s_battleEngine {int m388=0,m38C_battleMode=0,m22C_dragonCurrentQuadrant=0;struct {int m4=0;}m188_flags;Timer* m3CC=&timer;}engine;
struct Overlay {s_battleEngine* m4_battleEngine=&engine;}overlay;
struct Manager {Overlay* m10_battleOverlay=&overlay;}manager;
Manager* gBattleManager=&manager;
enum Call {None,Modal,Scroll,Confirm}; Call last=None;
bool active=false,ready=true,button=false,child=false,finished=false;
int BTL_T0_isAttackSeqActive(){return active;}
int BTL_T0_isBattleModeReady(){return ready;}
int tutorialController_checkD56cAndSetMode2(sTutorialControllerTask* s){
 last=Modal;if(active)return 0;child=active=true;finished=false;
 ++s->m1b_nextPhase;s->m1c_nextPhaseTarget=s->m1b_nextPhase+1;s->m10_timer=10;return 1;
}
int tutorialController_scrollAndAdvance(sTutorialControllerTask* s){
 last=Scroll;if(--s->m10_timer<1&&button){++s->m1b_nextPhase;s->m1c_nextPhaseTarget=s->m1b_nextPhase+1;s->m10_timer=10;return 1;}return 0;
}
int tutorialController_checkInputConfirm(sTutorialControllerTask* s){
 if(!button)return 0;child=false;finished=true;s->m1b_nextPhase=s->m1c_nextPhaseTarget;return 1;
}
int tutorialController_timerAndConfirm(sTutorialControllerTask* s){last=Confirm;if(--s->m10_timer<1){s->m10_timer=0;return tutorialController_checkInputConfirm(s);}return 0;}
void tutorialController_showPanel(sTutorialControllerTask*,int,int){}
void tutorialController_hidePanel(sTutorialControllerTask*){}
void tutorialController_setTextOverlay(sTutorialControllerTask*,int,int){}
int tutorialController_checkQuadrantAndSetMode(sTutorialControllerTask*,char,u8){return 0;}
// GENERATED_MOVEMENT_LESSON
int main(){
 // SH2 branch destinations: 5938A->A6E8, 59392->A72A, 5939A->A776.
 const int modal[]={1,4,0x13},scroll[]={5,6,7,8,9,0x14},confirm[]={2,0xa,0xf,0x15};
 for(int p:modal){sTutorialControllerTask s;s.m1b_nextPhase=p;last=None;active=false;button=false;tutorialPhase1(&s);assert(last==Modal);}
 // Phase 2 is deliberately NOT in the scroll set: it closes the input child.
 for(int p:scroll){sTutorialControllerTask s;s.m1b_nextPhase=p;last=None;button=false;tutorialPhase1(&s);assert(last==Scroll);}
 for(int p:confirm){sTutorialControllerTask s;s.m1b_nextPhase=p;last=None;button=false;tutorialPhase1(&s);assert(last==Confirm);}
 sTutorialControllerTask s;s.m1b_nextPhase=1;active=false;button=false;tutorialPhase1(&s);
 assert(s.m1b_nextPhase==2&&child&&active);
 for(int i=0;i<12;++i)tutorialPhase1(&s);
 assert(s.m1b_nextPhase==2&&child&&!finished);
 button=true;tutorialPhase1(&s);assert(s.m1b_nextPhase==3&&!child&&finished);
 // Native engine consumes attackAnimationFinished before the controller proceeds.
 active=false;button=false;tutorialPhase1(&s);assert(s.m1b_nextPhase==4);
 tutorialPhase1(&s);assert(s.m1b_nextPhase==5&&child&&active);
}

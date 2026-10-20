#include <cassert>
#include <cstdio>
#include <initializer_list>
struct Result { int m4=0,m72=6; };
struct {struct {struct {struct {int m6_buttonDown=0;} m0_current;} m0_inputDevices[1];} m4514;} graphicEngineStatus;
int exits=0;
void battleResultScreen_updateSub17(Result*){++exits;}
void update(Result* pThis){switch(pThis->m72){
#include "dismissal.inc"
case 7: break;
}}
int main(){
 for(int input:{0,2,4,6})for(int timer:{-1,0,1,90}){
  Result r{timer,6};exits=0;graphicEngineStatus.m4514.m0_inputDevices[0].m0_current.m6_buttonDown=input;
  update(&r);bool leave=input!=0||timer<=0;
  assert(exits==int(leave)&&r.m72==(leave?7:6));
 }
 puts("PASS generated result dismissal: input, timeout and simultaneous expiry exit exactly once; not device acceptance");
}

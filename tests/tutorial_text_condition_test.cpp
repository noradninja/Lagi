#include <cstdint>
#include <cassert>
#include <cstdio>
#include <initializer_list>
using s8=std::int8_t;using u8=std::uint8_t;
struct sTutorialControllerTask {unsigned m20_flags=0;unsigned m18_textSet=0;};
struct Engine {unsigned m38C_battleMode=0;s8 m38D_battleSubMode=0,m22C_dragonCurrentQuadrant=0;}engine;
struct Menu {unsigned m20=0;}menu;
struct Hud {Menu* m28_battleCommandMenu=&menu;}hud;
struct Overlay {Hud* m20_battleHud=&hud;Engine* m4_battleEngine=&engine;}overlay;
struct Manager {Overlay* m10_battleOverlay=&overlay;}manager;
Manager* gBattleManager=&manager;
unsigned clears=0;void tutorialController_clearTextArea(){++clears;}
#include "../src/integration/lagi_tutorial_text_condition.inc"
int main(){
 unsigned cases=0;
 for(unsigned text=0;text<11;++text)for(unsigned flags=0;flags<32;++flags)
 for(unsigned menuState=0;menuState<3;++menuState)for(unsigned mode=0;mode<17;++mode)
 for(int sub:{-128,-1,0,2,3,127})for(int q=0;q<4;++q){
  sTutorialControllerTask task{flags,text};clears=0;engine={mode,s8(sub),s8(q)};
  hud.m28_battleCommandMenu=menuState?&menu:nullptr;menu.m20=menuState==2?8:0;
  unsigned expected=flags,expectedClears=0;
  if(menuState!=2)expected&=0xEF;
  else if(!(flags&16)){expected|=16;++expectedClears;}
  bool finished=false;
  if(text<4)finished=q==int(text);
  else if(text==4)finished=mode==0 && sub>=3;
  else if(text==5)finished=mode==3 && sub>=0;
  else if(text==6)finished=mode==1 && sub>=0;
  else if(text==7)finished=mode==4 && sub>=0;
  else if(text==8){switch(mode){case 0:finished=sub>=3;break;case 1:case 3:case 4:finished=sub>=0;break;}}
  if(finished){expected&=text<4?0xE7:0xF7;++expectedClears;}
  tutorialController_checkTextCondition(&task);
  assert(task.m20_flags==expected && clears==expectedClears && task.m18_textSet==text);
  ++cases;
 }
 std::printf("PASS native tutorial text conditions: %u prompt/flag/menu/mode/signed-submode/quadrant combinations; no device validation\n",cases);
}

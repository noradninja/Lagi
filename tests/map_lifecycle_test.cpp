#include "lagi/azel_map_menu.h"
#include <cassert>
#include <cstdio>
int main(){
 LagiAzelMapLifecycle state;
 for(int i=0;i<15;++i) {auto step=lagiAzelAdvanceMapLifecycle(state);assert(step.action==LagiAzelMapAction::None && state.phase==0);}
 lagiAzelAdvanceMapLifecycle(state);assert(state.phase==1 && state.countdown==0);
 auto step=lagiAzelAdvanceMapLifecycle(state);assert(step.action==LagiAzelMapAction::SetupSelection && state.phase==2);
 for(int mode=0;mode<3;++mode){state.mode=mode;step=lagiAzelAdvanceMapLifecycle(state);assert(int(step.action)==int(LagiAzelMapAction::InputSelection)+mode && state.phase==2);}
 state.phase=3;step=lagiAzelAdvanceMapLifecycle(state);assert(step.action==LagiAzelMapAction::CloseMenu && state.phase==4 && state.countdown==15);
 for(int i=0;i<14;++i)assert(!lagiAzelAdvanceMapLifecycle(state).finished);
 step=lagiAzelAdvanceMapLifecycle(state);assert(step.action==LagiAzelMapAction::Finish && step.finished);
 for(int available=-1;available<=1;++available){
  state={true,1,0,0,available};step=lagiAzelAdvanceMapLifecycle(state);
  assert(step.action==(available==1?LagiAzelMapAction::SetupStandaloneMode1:LagiAzelMapAction::SetupSelection));
  state.phase=3;step=lagiAzelAdvanceMapLifecycle(state);assert(step.action==LagiAzelMapAction::CloseStandalone && step.finished);
 }
 puts("PASS: native Map opening/selection/input routing/closing timing; service execution and native task hookup untested");
}

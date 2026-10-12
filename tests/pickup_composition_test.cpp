#include <cassert>
#include <cstdio>
#include <initializer_list>
enum class Vdp1UiPass {All,Background,WorldEffects,Foreground};
#include "pickup_pass_production.inc"
int main(){
 for(bool background:{false,true}) for(bool world:{false,true}) {
  assert(vdp1UiPassAccepts(Vdp1UiPass::All,background,world));
  int count=0;
  for(auto pass:{Vdp1UiPass::Background,Vdp1UiPass::WorldEffects,Vdp1UiPass::Foreground})
   count+=vdp1UiPassAccepts(pass,background,world);
  assert(count==1);
  assert(vdp1UiPassAccepts(Vdp1UiPass::WorldEffects,background,world)==(!background && world));
  assert(vdp1UiPassAccepts(Vdp1UiPass::Foreground,background,world)==(!background && !world));
 }
 puts("PASS: actual production pass partition; each command draws once, effects separated from overlays");
}

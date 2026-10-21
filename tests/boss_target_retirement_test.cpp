#include <vector>
#include <cassert>
#include <cstdio>
using s32=int;
struct Target{unsigned m50_flags=0x200000;};
struct sArachnothSubModel{int m64=0;std::vector<Target> m58_targetables;};
struct sArachnothFormation{sArachnothSubModel m8_normalBody,m98_poisonDart,m128_eatDragonBody;};
struct Flags{int m2_needToSortEnemiesByDistanceFromDragon=0;};
struct Engine{Flags m188_flags;};
struct Overlay{Engine* m4_battleEngine;};
struct Manager{Overlay* m10_battleOverlay;};
Engine engine;Overlay overlay{&engine};Manager manager{&overlay},*gBattleManager=&manager;
#include "production.inc"
int main(){
 sArachnothFormation boss;
 boss.m8_normalBody.m64=3;boss.m8_normalBody.m58_targetables.resize(3);
 boss.m98_poisonDart.m64=2;boss.m98_poisonDart.m58_targetables.resize(2);
 boss.m128_eatDragonBody.m64=4;boss.m128_eatDragonBody.m58_targetables.resize(4);
 for(int repetition=0;repetition<2;++repetition){
  arachnoth_disableAllTargetables(&boss);
  for(auto* form:{&boss.m8_normalBody,&boss.m98_poisonDart,&boss.m128_eatDragonBody})
   for(const auto& target:form->m58_targetables)assert(target.m50_flags&0x40000);
  assert(engine.m188_flags.m2_needToSortEnemiesByDistanceFromDragon==1);
 }
 puts("PASS all boss forms retired through existing targetable deletion flags, including hidden forms; idempotent cleanup");
}

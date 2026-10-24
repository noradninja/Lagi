#include "lagi/battle_loading_cover.h"
#include <cassert>
#include <memory>
#include <vector>
#include <cstdio>
static std::vector<int> g_submissions,g_publishedSubmissions,g_vdp1UiCommands,g_publishedVdp1UiCommands;
static std::vector<std::shared_ptr<int>> g_adaptedModels,g_publishedAdaptedModels;
static unsigned g_modelCacheMisses=0,g_publishedModelCacheMisses=0;
static unsigned g_explicitStaticContextsSet=0,g_publishedExplicitStaticContextsSet=0;
static unsigned g_explicitStaticContextsConsumed=0,g_publishedExplicitStaticContextsConsumed=0;
static unsigned g_publishedFrameNumber=0;
static void publish(bool retainPreviousScene){
#include "publication.inc"
}
int main(){
 lagi::BattleLoadingCover cover;
 assert(!cover.retainPreviousScene(0));
 assert(cover.allowLoad(false,0,0,0)); // Normal nonbattle publications unchanged.
 g_submissions={1,2};g_vdp1UiCommands={3};g_adaptedModels.push_back(std::make_shared<int>(4));
 std::weak_ptr<int> old=g_adaptedModels[0];publish(false);
 g_submissions.clear();g_vdp1UiCommands.clear();g_adaptedModels.clear();
 cover.begin();
 for(int fade=0;fade<255;++fade){
  assert(!cover.allowLoad(false,fade,fade,fade));
  publish(cover.retainPreviousScene(0));
  assert((g_publishedSubmissions==std::vector<int>{1,2}));
  assert((g_publishedVdp1UiCommands==std::vector<int>{3}));assert(!old.expired());
 }
 assert(!cover.allowLoad(false,255,255,255));
 assert(!cover.allowLoad(true,255,254,255));
 assert(cover.allowLoad(true,255,255,255));
 assert(cover.retainPreviousScene(12,false)); // Unrelated geometry during sound loading is not battle readiness.
 for(int i=0;i<200;++i){assert(cover.retainPreviousScene(0));publish(true);assert(!old.expired());}
 g_submissions={9};g_vdp1UiCommands={10};g_adaptedModels.push_back(std::make_shared<int>(11));
 assert(!cover.retainPreviousScene(1));publish(false);
 assert(old.expired() && g_publishedSubmissions[0]==9 && g_publishedVdp1UiCommands[0]==10);
 assert(*g_publishedAdaptedModels[0]==11);
 cover.begin();assert(cover.active && !cover.loading); // Repeated encounters restart correctly.
 assert(cover.retainPreviousScene(12)); // Outgoing geometry is not readiness.
 assert(g_publishedFrameNumber==457); // Cover still publishes; no frozen fade clock.
 std::puts("PASS loading cover: no early load, exact outgoing publication/ownership retained, white hold through empty frames, incoming scene release and repeat entry; mocked renderer, not Vita fade proof");
}

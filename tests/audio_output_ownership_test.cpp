#include "lagi/audio_output_owner.h"
#include <array>
#include <atomic>
#include <cassert>
#include <thread>
#include <vector>
#include <cstdio>
using s32=int;using u8=unsigned char;
struct Sound {int volume=0;};
std::array<Sound,8> g_activeSounds;
std::array<std::atomic<int>,8> g_activeSoundIds,g_activeSoundSequences;
std::atomic<int> g_requestedSequence{68};
bool g_gameplayRenderEnabled=true;
struct Command {int type,slot,volume;};std::vector<Command> commands;
void queue_command(int type,int slot,int volume=0,int=0){commands.push_back({type,slot,volume});}
#include "production.inc"
int main(){
 worker_fade_all();assert(g_gameplayRenderEnabled&&commands.size()==16);
 for(int i=0;i<8;++i){assert(commands[i*2].type==2&&commands[i*2].slot==i);
  assert(commands[i*2+1].type==5&&commands[i*2+1].volume==127);
  assert(g_activeSounds[i].volume==127);}
 for(int i=0;i<8;++i){g_activeSoundIds[i]=-1;g_activeSoundSequences[i]=-1;}
 g_activeSoundIds[0]=100;g_activeSoundSequences[0]=68;assert(findSound(100)==0);
 g_requestedSequence=67;assert(findSound(100)==-1); // no cinematic ID alias in boss bank
 g_activeSoundSequences[0]=67;assert(findSound(100)==0);
 g_activeSoundIds[1]=2;g_activeSoundSequences[1]=68;assert(findSound(2)==1);
 std::atomic<bool> entered{false},release{false},claimed{false};
 std::thread native([&]{lagi::audio::OutputLease lease;entered=true;while(!release.load())std::this_thread::yield();});
 while(!entered.load())std::this_thread::yield();
 std::thread movie([&]{lagi::audio::claim_movie_output();claimed=true;});
 while(!lagi::audio::movieOwnsOutput.load())std::this_thread::yield();
 assert(!claimed.load());release=true;native.join();movie.join();assert(claimed.load());
 lagi::audio::release_movie_output();assert(!lagi::audio::movieOwnsOutput.load());
 puts("PASS production fade commands preserve output, bank-local sound IDs do not alias, system IDs remain global, movie port handoff waits for native quantum; no listening acceptance");
}

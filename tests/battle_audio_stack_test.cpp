#include "lagi/native_audio_stack.h"
#include <array>
#include <atomic>
#include <cassert>
#include <cstdio>
#include <vector>
using s8=std::int8_t;using s16=std::int16_t;using s32=std::int32_t;using u8=std::uint8_t;using u32=std::uint32_t;
struct ActiveSound{s16 soundIndex=-1,volume=0x80;};
struct PendingSound{s32 soundIndex,bankIndex,volume,arg;};
struct Ptr{unsigned offset=0;Ptr operator+(unsigned n)const{return {offset+n};}};
struct sSequenceConfig{unsigned mE=0;Ptr m4_soundConfigs;};
std::vector<sSequenceConfig> SoundDataTable(8);
const sSequenceConfig* g_sequence=nullptr;
s32 g_sequenceNumber=-1,g_loadingState=-1;unsigned g_audioDiagBudget=0,g_audioPerfBudget=0;
std::atomic<bool> g_loadingFinished{true};bool g_gameplayRenderEnabled=false;
std::vector<PendingSound> g_pendingSounds,submitted;
std::vector<int> g_commands;
std::array<ActiveSound,8> g_activeSounds{},g_resumeSounds{};
std::array<std::atomic<s32>,8> g_activeSoundIds{};
lagi::NativeAudioStack<std::array<ActiveSound,8>> g_sequenceStack;
bool g_resumeAfterLoad=false;
enum class AudioEventType{LoadBanks,StopBanks,Sound,FadeAll,BattleMark,PopSequence};
struct AudioEvent{AudioEventType type=AudioEventType::Sound;int a=0,b=0,c=0,d=0;};
constexpr unsigned kAudioEventCapacity=256,kAudioEventMask=255;
std::array<AudioEvent,kAudioEventCapacity> g_audioEvents{};
std::atomic<unsigned> g_audioEventRead{0},g_audioEventWrite{0},g_pendingSequenceEvents{0};
struct Common{Ptr getSaturnPtr(unsigned address){return {address};}}common;
Common* gCommonFile=&common;
std::array<s8,64> configs{};
s8 readSaturnS8(Ptr p){assert(p.offset<configs.size());return configs[p.offset];}
void reset_active_sounds(){g_activeSounds={};for(auto& id:g_activeSoundIds)id.store(-1);}
void queue_command(int,int){}
void ensure_output_stream(){}
int sceKernelGetCpuId(){return 2;}
unsigned next_audio_trace(){return 0;}
namespace lagi::platform::logging{template<class... T>void writef(const char*,T...){}}
void submit_sound_request(const PendingSound& sound){submitted.push_back(sound);}
#include "load.inc"
#include "pop.inc"
#include "replay.inc"
void worker_fade_all(){}
#include "queue.inc"
#include "events.inc"
#include "ready.inc"
int main(){
    worker_load_banks(2,0);assert(g_sequenceNumber==2 && g_sequenceStack.index==0);
    g_activeSounds[0]={100,100};g_activeSounds[1]={101,80};
    SoundDataTable[2].mE=1;g_sequenceStack.mark();
    worker_load_banks(4,0);
    assert(g_sequenceStack.index==1 && g_sequenceStack.entries[0].sequence==3);
    assert(!g_sequenceStack.entries[0].marked && g_sequenceStack.entries[0].sounds[0].soundIndex==100);
    assert(g_sequenceStack.entries[0].sounds[0].volume==0x80);
    worker_pop_sequence(1);
    assert(g_sequenceNumber==3 && g_sequenceStack.index==0 && g_resumeAfterLoad && !g_loadingFinished.load());
    configs[2]=1;configs[6]=0;g_loadingState=-1;replay_restored_sequence();
    assert(submitted.size()==1 && submitted[0].soundIndex==100 && submitted[0].volume==127);
    assert(g_activeSounds[1].soundIndex==101 && g_activeSoundIds[1].load()==101);
    replay_restored_sequence();assert(submitted.size()==1);
    g_sequenceStack.mark();worker_pop_sequence(1); // cancel before bank switch
    assert(g_sequenceStack.index==0 && g_sequenceNumber==3 && g_loadingFinished.load());
    worker_pop_sequence(1);assert(g_sequenceStack.index==0 && g_sequenceNumber==3);
    g_sequenceStack.mark();worker_load_banks(4,0);worker_pop_sequence(0);
    assert(g_sequenceNumber==3 && !g_resumeAfterLoad);replay_restored_sequence();assert(submitted.size()==1);
    lagi::NativeAudioStack<std::array<ActiveSound,8>> stack;
    assert(stack.load(1,0,0,g_activeSounds));
    for(unsigned i=1;i<8;++i){stack.mark();assert(stack.load(2,0,0,g_activeSounds));assert(stack.index==i);}
    stack.mark();assert(!stack.load(3,0,0,g_activeSounds) && stack.index==7);
    assert(stack.pop()==decltype(stack)::Pop::canceled);
    for(unsigned i=7;i>0;--i){assert(stack.pop()==decltype(stack)::Pop::restore);assert(stack.index==i-1);}
    assert(stack.pop()==decltype(stack)::Pop::empty);
    assert(stack.load(5,0,0,g_activeSounds));stack.mark();assert(stack.load(2,0,-1,g_activeSounds));
    assert(stack.entries[0].sequence==4);
    // An old completion cannot acknowledge a newer accepted bank/pop event.
    g_loadingFinished.store(true);g_loadingState=-1;
    assert(push_audio_event({AudioEventType::BattleMark}));
    assert(push_audio_event({AudioEventType::LoadBanks,4,0}));
    assert(!isSoundLoadingFinished() && g_pendingSequenceEvents.load()==1);
    process_audio_events();assert(g_pendingSequenceEvents.load()==0 && !isSoundLoadingFinished());
    g_loadingFinished.store(true);g_loadingState=-1;
    assert(push_audio_event({AudioEventType::PopSequence,1}));
    assert(!isSoundLoadingFinished());process_audio_events();assert(!isSoundLoadingFinished());
    assert(g_sequenceNumber==3 && g_sequenceStack.index==0);
    for(unsigned i=0;i<kAudioEventCapacity;++i)assert(push_audio_event({AudioEventType::Sound}));
    assert(!push_audio_event({AudioEventType::PopSequence,1}) && g_pendingSequenceEvents.load()==0);
    process_audio_events();assert(g_pendingSequenceEvents.load()==0);
    puts("PASS: production audio worker stack push/pop, parent mark clearing, return offset, sentinel volumes, config-gated replay, cancel/no-replay, nesting and bounds; mocked SCSP/worker scheduling");
}

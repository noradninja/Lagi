#include "lagi/native_audio_stack.h"
#include "lagi/native_audio_banks.h"
#include <array>
#include <atomic>
#include <cassert>
#include <cstdio>
#include <vector>
#include <cstring>
using s8=std::int8_t;using s16=std::int16_t;using s32=std::int32_t;using u8=std::uint8_t;using u16=std::uint16_t;using u32=std::uint32_t;
struct ActiveSound{s16 soundIndex=-1,volume=0x80;};
struct PendingSound{s32 soundIndex,bankIndex,volume,arg;};
struct Ptr{unsigned offset=0xFFFFFFFF;Ptr operator+(unsigned n)const{return {offset+n};}Ptr& operator+=(unsigned n){offset+=n;return *this;}bool isNull()const{return offset==0xFFFFFFFF;}};
using sSaturnPtr=Ptr;
struct sSequenceConfig{unsigned mE=0;Ptr m4_soundConfigs{0};int m8_areaMapIndex=0;Ptr m0{};unsigned mC_numMapEntries=0;};
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
lagi::NativeAudioBankResidency g_bankResidency;
const sSequenceConfig* g_previousSequence=nullptr;
std::array<ActiveSound,8> g_transitionSounds{};u8 g_stoppedBankMask=0;
enum class AudioEventType{LoadBanks,StopBanks,Sound,FadeAll,BattleMark,PopSequence};
struct AudioEvent{AudioEventType type=AudioEventType::Sound;int a=0,b=0,c=0,d=0;};
constexpr unsigned kAudioEventCapacity=256,kAudioEventMask=255;
std::array<AudioEvent,kAudioEventCapacity> g_audioEvents{};
std::atomic<unsigned> g_audioEventRead{0},g_audioEventWrite{0},g_pendingSequenceEvents{0};
struct Common{Ptr getSaturnPtr(unsigned address){return {address};}}common;
Common* gCommonFile=&common;
std::array<s8,512> configs{};
s8 readSaturnS8(Ptr p){assert(p.offset<configs.size());return configs[p.offset];}
u32 readSaturnU32(Ptr p){u32 result=0;for(unsigned i=0;i<4;++i)result=result<<8|u8(readSaturnS8(p+i));return result;}
Ptr readSaturnEA(Ptr p){return {readSaturnU32(p)};}
std::string readSaturnString(Ptr p){return reinterpret_cast<const char*>(configs.data()+p.offset);}
namespace lagi::disc{int selected_disc(){return 1;}}
std::array<u16,8> status{};
unsigned m68k_read_memory_16(unsigned address){assert(address>=0x780 && address<0x790);return status[(address-0x780)/2];}
void m68k_write_memory_8(unsigned,unsigned){}
struct ScspCommand{s8 bytes[16]{};};
void write_driver_command(const ScspCommand&,unsigned){}
bool bankReadSuccess=true;unsigned bankLoads=0;
bool load_current_banks(){++bankLoads;return bankReadSuccess;}
void reset_active_sounds(){g_activeSounds={};for(auto& id:g_activeSoundIds)id.store(-1);}
std::vector<int> stops;
void queue_command(int command,int slot){assert(command==2);stops.push_back(slot);}
void ensure_output_stream(){}
int sceKernelGetCpuId(){return 2;}
unsigned next_audio_trace(){return 0;}
namespace lagi::platform::logging{template<class... T>void writef(const char*,T...){}}
void submit_sound_request(const PendingSound& sound){submitted.push_back(sound);}
struct RecordedCommand{int command,slot,bank,sound;};
std::vector<RecordedCommand> requestCommands;
void queue_command(u8 command,u8 slot,u8 bank,u8 sound=0){requestCommands.push_back({command,slot,bank,sound});}
Ptr sound_config_for(s16 sound){return g_sequence->m4_soundConfigs+unsigned(sound-100)*4;}
#include "request.inc"
#include "load.inc"
#include "pop.inc"
#include "replay.inc"
#include "transition.inc"
#include "advance.inc"
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
    configs[2]=1;configs[6]=0;g_stoppedBankMask=1;g_loadingState=-1;replay_restored_sequence();
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
    assert(lagi::native_audio_load_state(-1,0,2,0)==0);
    assert(lagi::native_audio_load_state(1,3,2,4)==0);
    assert(lagi::native_audio_load_state(1,3,2,3)==2);
    assert(lagi::native_audio_load_state(2,3,2,3)==8);
    g_bankResidency.select_disc(1);g_bankResidency.loaded("old.bnk",100,20);
    assert(g_bankResidency.contains("OLD.BNK",100));g_bankResidency.loaded("new.bnk",110,5);
    assert(!g_bankResidency.contains("old.bnk",100));
    g_bankResidency.select_disc(2);assert(g_bankResidency.files.empty());
    // Production stop-mask/driver wait: bank 1 stays active, only bank 0 stops.
    configs.fill(0);std::memcpy(configs.data()+200,"NEW.BNK",8);
    auto put=[](unsigned at,u32 value){for(unsigned i=0;i<4;++i)configs[at+i]=s8(value>>((3-i)*8));};
    put(100,200);put(104,0x25A10000);put(108,1);
    SoundDataTable[3].m0={100};g_sequence=&SoundDataTable[3];g_previousSequence=g_sequence;
    g_transitionSounds={};g_transitionSounds[0].soundIndex=100;g_transitionSounds[1].soundIndex=101;
    configs[0]=0;configs[4]=1;status={0x100,0x100};g_stoppedBankMask=0;stops.clear();
    unsigned commandSlot=0;g_loadingState=2;advance_loading_state(commandSlot);
    assert(g_loadingState==3 && g_stoppedBankMask==1 && stops==std::vector<int>{0});
    advance_loading_state(commandSlot);assert(g_loadingState==3 && bankLoads==0);
    status[0]=0;advance_loading_state(commandSlot);assert(g_loadingState==4);
    bankReadSuccess=false;advance_loading_state(commandSlot);assert(g_loadingState==4 && bankLoads==1);
    bankReadSuccess=true;advance_loading_state(commandSlot);assert(g_loadingState==5 && bankLoads==2);
    g_bankResidency.loaded("NEW.BNK",0x25A10000,20);g_stoppedBankMask=0;stops.clear();stop_changed_sequence_banks();
    assert(g_stoppedBankMask==0 && stops.empty());
    submitted.clear();configs[2]=1;configs[6]=1;
    g_resumeSounds={};g_resumeSounds[0].soundIndex=100;g_resumeSounds[1].soundIndex=101;
    g_resumeAfterLoad=true;g_stoppedBankMask=1;replay_restored_sequence();
    assert(submitted.size()==1 && submitted[0].soundIndex==100);
    submitted.clear();g_resumeAfterLoad=true;g_stoppedBankMask=2;replay_restored_sequence();
    assert(submitted.size()==1 && submitted[0].soundIndex==101);
    for(unsigned word=0;word<65536;++word)assert(lagi::native_audio_bank_playing(u16(word))==(word>=256));
    // Tunnel transition: simultaneous ambience uses one data bank but
    // distinct playback slots. Status/stop must follow those control slots.
    reset_active_sounds();requestCommands.clear();configs[0]=3;configs[1]=5;configs[3]=4;
    configs[4]=3;configs[5]=6;configs[7]=5;
    native_submit_sound_request({100,1,100,0});
    native_submit_sound_request({101,1,110,0});
    assert(g_activeSounds[4].soundIndex==100 && g_activeSounds[5].soundIndex==101);
    assert(g_activeSoundIds[4].load()==100 && g_activeSoundIds[5].load()==101);
    assert(g_activeSounds[3].soundIndex==-1);
    assert(requestCommands.size()==4 && requestCommands[1].command==1 && requestCommands[1].slot==4 && requestCommands[1].bank==3);
    native_submit_sound_request({100,5,60,2});
    assert(requestCommands.back().command==5 && requestCommands.back().slot==4 && g_activeSounds[4].volume==60);
    native_submit_sound_request({101,2,0,0});assert(stops.back()==5);
    g_transitionSounds=g_activeSounds;g_previousSequence=g_sequence;
    g_bankResidency.files.clear();put(108,8);g_stoppedBankMask=0;status={};status[4]=status[5]=0x100;stops.clear();
    stop_changed_sequence_banks();assert(g_stoppedBankMask==8 && stops==std::vector<int>({4,5}));
    assert(!changed_sequence_banks_stopped());status[4]=0;assert(!changed_sequence_banks_stopped());status[5]=0;
    assert(changed_sequence_banks_stopped());
    puts("PASS: production sound requests retain distinct playback-slot ownership for a shared data bank; volume/stop commands and transition stop/wait target the actual driver slots.");
    puts("PASS: production audio stack/queue/readiness plus native loader choice, residency mask, selective stop, driver-status wait and transfer-failure retention; mocked SCSP/worker scheduling");
}

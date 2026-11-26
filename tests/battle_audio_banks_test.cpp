#include "lagi/audio_diagnostics.h"
#include "lagi/native_audio_banks.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <map>
using u8=std::uint8_t;using u32=std::uint32_t;
u8 sat_ram[0x80000]{};
std::array<u8,256> config{};
struct sSaturnPtr{unsigned at=0;bool isNull()const{return !at;}sSaturnPtr operator+(unsigned n)const{return {at+n};}sSaturnPtr& operator+=(unsigned n){at+=n;return *this;}};
u32 readSaturnU32(sSaturnPtr p){u32 v=0;for(unsigned i=0;i<4;++i)v=v<<8|config[p.at+i];return v;}
sSaturnPtr readSaturnEA(sSaturnPtr p){return {readSaturnU32(p)};}
std::string readSaturnString(sSaturnPtr p){return reinterpret_cast<const char*>(config.data()+p.at);}
int selectedDisc=1,reads=0;
std::map<std::string,std::vector<u8>> files{{"A.BNK",{1,2,3}},{"B.BNK",{4,5,6}}};
namespace lagi::disc {
int selected_disc(){return selectedDisc;}
bool read_file(const char* name,std::vector<u8>& bytes){++reads;const auto found=files.find(name);if(found==files.end())return false;bytes=found->second;return true;}
}
namespace lagi::platform::logging{template<class... T>void writef(const char*,T...) {}}
unsigned next_audio_trace(){return 0;}
lagi::NativeAudioBankResidency g_bankResidency;
// Diagnostics added in Step 2B are intentionally mocked in this loader test.
std::size_t g_dspImageOffset=0,g_dspImageBytes=0;
std::uint32_t g_dspImageLoadedHash=0;
std::uint32_t dsp_image_hash(const void*,std::size_t){return 0;}
void trace_dsp_image(const char*){}
#include "load.inc"
void select(const char* name,u32 address){
    config.fill(0);std::strcpy(reinterpret_cast<char*>(config.data()+128),name);
    auto put=[](unsigned at,u32 v){for(unsigned i=0;i<4;++i)config[at+i]=u8(v>>((3-i)*8));};
    put(8,128);put(12,address);
}
int main(){
    select("A.BNK",0x25A00010);assert(load_sound_file_list({8}) && reads==1 && sat_ram[16]==1);
    assert(load_sound_file_list({8}) && reads==1);
    select("B.BNK",0x25A00011);assert(load_sound_file_list({8}) && reads==2 && sat_ram[17]==4);
    assert(!g_bankResidency.contains("A.BNK",0x25A00010));
    select("A.BNK",0x25A00010);assert(load_sound_file_list({8}) && reads==3 && sat_ram[17]==2);
    select("MISSING.BNK",0x25A00010);assert(!load_sound_file_list({8}) && sat_ram[16]==1);
    files["BIG.BNK"]=std::vector<u8>(4,99);select("BIG.BNK",0x25A7FFFE);
    assert(!load_sound_file_list({8}) && sat_ram[0x7FFFE]==0);
    select("A.BNK",0x25900000);const auto oldReads=reads;
    assert(!load_sound_file_list({8}) && reads==oldReads);
    select("A.BNK",0x25A00010);++selectedDisc;
    assert(load_sound_file_list({8}) && reads==oldReads+1);
    // Original Disc 1 layout: the area-1 work ring covers both area-0 EXBs.
    // Only the relevant map records are needed for this synthetic fixture.
    const u8 maps[]={0x30,0x05,0x40,0,0,1,1,0,0xFF,
                     0x30,0x04,0x80,0,0,1,1,0,0xFF,0xFF};
    assert(g_bankResidency.set_area_maps(maps,sizeof(maps)));
    assert(g_bankResidency.activate_area(0)==0);
    files["A5DSP.EXB"]=std::vector<u8>(0x540,0x11);
    files["BTL1DSP.EXB"]=std::vector<u8>(0x540,0x22);
    select("A5DSP.EXB",0x25A53500);assert(load_sound_file_list({8}));
    select("BTL1DSP.EXB",0x25A53A40);assert(load_sound_file_list({8}));
    const int beforeHit=reads;
    assert(load_sound_file_list({8}) && reads==beforeHit);
    // Same-area mutation must be caught even without an area-map command.
    sat_ram[0x53A40+0x140]=0xE7;
    assert(load_sound_file_list({8}) && reads==beforeHit+1 && sat_ram[0x53A40+0x140]==0x22);
    assert(g_bankResidency.activate_area(1)==2);
    assert(!g_bankResidency.contains("A5DSP.EXB",0x25A53500));
    assert(!g_bankResidency.contains("BTL1DSP.EXB",0x25A53A40));
    assert(g_bankResidency.contains("A.BNK",0x25A00010));
    std::memset(sat_ram+0x48000,0xE7,0x10100);
    assert(g_bankResidency.activate_area(0)==0);
    const int beforeReturn=reads;
    select("A5DSP.EXB",0x25A53500);assert(load_sound_file_list({8}));
    select("BTL1DSP.EXB",0x25A53A40);assert(load_sound_file_list({8}));
    assert(reads==beforeReturn+2 && sat_ram[0x53500]==0x11 && sat_ram[0x53A40]==0x22);
    // No identity may be retained for a newly loaded file in live work RAM.
    g_bankResidency.loaded("WORK.BNK",0x25A54000,32);
    assert(!g_bankResidency.contains("WORK.BNK",0x25A54000));
    assert(g_bankResidency.activate_area(99)==3 && g_bankResidency.files.empty());
    assert(!g_bankResidency.set_area_maps(maps,sizeof(maps)-1));
    puts("PASS: area-map DSP work RAM invalidation, immutable EXB byte check and reload, unknown-map fallback");
    puts("PASS: production sound-bank loader skips exact resident files, invalidates partial overlaps, rejects missing/oversize/out-of-range writes and reloads after disc change; mocked disc bytes");
}

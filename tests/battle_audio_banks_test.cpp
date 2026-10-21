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
    puts("PASS: production sound-bank loader skips exact resident files, invalidates partial overlaps, rejects missing/oversize/out-of-range writes and reloads after disc change; mocked disc bytes");
}

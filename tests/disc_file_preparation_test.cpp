#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>
#include "lagi/prepared_disc_files.h"
using u8=std::uint8_t;using u16=std::uint16_t;using u32=std::uint32_t;
struct sSaturnPtr {const char* name=nullptr;bool isNull()const{return !name;}};
std::string readSaturnString(sSaturnPtr p){return p.name;}
struct s_fileBundle {
    u8* raw;explicit s_fileBundle(u8* p):raw(p){}
    ~s_fileBundle(){delete[] raw;}
    void setPatchFilePointers(u16){}
};
struct sSaturnMemoryFile {
    sSaturnMemoryFile(const char*,u32);
    ~sSaturnMemoryFile(){delete[] m_data;}
    std::string m_name;u8* m_data;u32 m_dataSize,m_base;
};
struct {int m0=0;} fileInfoStruct;
namespace lagi::platform::logging {template<class... T>void writef(const char*,T...) {}}
namespace lagi::disc {
int disc=1,reads=0;std::string failed;
int selected_disc(){return disc;}
bool has_file(const char* name){return name&&failed!=name;}
bool file_size(const char* name,u32& size){size=4;return has_file(name);}
bool read_file(const char* name,std::vector<u8>& out){
    ++reads;
    if(!has_file(name))return false;
    out.assign(4,u8(reads));return true;
}
}
#include "production.inc"
int main(){
    using namespace lagi::disc;
    assert(prepare_native_files({"FLD_A5.FNT","FLD_A5.PRG"})&&reads==2);
    u8 font[4]{};
    assert(loadFile("FLD_A5.FNT",static_cast<u8*>(nullptr),0)==-1);
    assert(loadFile("FLD_A5.FNT",font,0)==0&&font[0]==1&&reads==2);
    assert(isSoundFileLoaded({"FLD_A5.FNT"}));
    {
        sSaturnMemoryFile program("FLD_A5.PRG",0x06054000);
        assert(program.m_dataSize==4&&program.m_data[0]==2&&reads==2);
    }
    assert(loadFile("FLD_A5.FNT",font,0)==0&&reads==3&&font[0]==3);
    // Partial failed preparation cannot leak retained bytes to later loaders.
    failed="FLD_A5.PRG";
    assert(!prepare_native_files({"FLD_A5.FNT","FLD_A5.PRG"})&&reads==5);
    failed.clear();assert(loadFile("FLD_A5.FNT",font,0)==0&&reads==6&&font[0]==6);
    assert(prepare_native_files({"FLD_A5.FNT"})&&reads==7);
    disc=2;assert(loadFile("FLD_A5.FNT",font,0)==0&&reads==8&&font[0]==8);
    assert(prepare_native_files({"FLD_A5.PRG"})&&reads==9);
    clear_prepared_native_files();
    sSaturnMemoryFile fresh("FLD_A5.PRG",0x06054000);
    assert(reads==10&&fresh.m_data[0]==10);
    // Preparation is not a completed destination copy or sound-file transfer.
    assert(prepare_native_files({"OTHER.FNT"}));
    assert(!isSoundFileLoaded({"OTHER.FNT"}));
    assert(loadFile("OTHER.FNT",font,0)==0&&isSoundFileLoaded({"OTHER.FNT"}));
    puts("PASS production file adapter consumes prepared font/overlay bytes once, preserves completion, clears failure/disc-change/cancellation state");
}

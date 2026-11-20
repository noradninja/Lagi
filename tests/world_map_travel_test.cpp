#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include "lagi/prepared_disc_files.h"
using s8=std::int8_t;
struct sSaturnPtr {
    unsigned ea=0;
    bool isNull() const {return !ea;}
    sSaturnPtr operator+(unsigned offset)const{return {ea+offset};}
};
struct Common {sSaturnPtr getSaturnPtr(unsigned ea){return {ea};}} common,*gCommonFile=&common;
std::vector<std::uint8_t> memory(0x20000);
s8 readSaturnS8(sSaturnPtr p){return s8(memory.at(p.ea-0x200000));}
sSaturnPtr readSaturnEA(sSaturnPtr p){
    unsigned value=0;
    for(unsigned i=0;i<4;++i)value=(value<<8)|memory.at(p.ea-0x200000+i);
    return {value};
}
std::string readSaturnString(sSaturnPtr p){return reinterpret_cast<const char*>(memory.data()+p.ea-0x200000);}
void put(unsigned ea,unsigned value){
    for(unsigned i=0;i<4;++i)memory.at(ea-0x200000+i)=std::uint8_t(value>>(24-i*8));
}
void text(unsigned ea,const char* value){std::memcpy(memory.data()+ea-0x200000,value,std::strlen(value)+1);}
namespace lagi::worldmap {
struct MapDestination {
    int storyBit,gameStatus;s8 preloadIndex,type;
    bool canTravel()const{return gameStatus>=0&&type>=0&&type<=3;}
};
}
namespace lagi::disc {
PreparedFiles prepared;
std::vector<std::string> reads;
std::string failed;
bool prepare_native_files(const std::vector<std::string>& names){
    return prepared.prepare(1,names,[](const char* name,std::vector<std::uint8_t>& bytes){
        reads.emplace_back(name);
        bytes.assign(16,std::uint8_t(reads.size()));
        return failed!=name;
    });
}
}
#include "production.inc"
int main(){
    using namespace lagi::worldmap;
    using namespace lagi::disc;
    text(0x210000,"FLD_A5.FNT");text(0x210020,"FLD_A5.PRG");
    put(0x208C58+3*20+8,0x210000);put(0x208C58+3*20+4,0x210020);
    MapDestination garil{0x6C,0x56,3,0};
    assert(prepare_map_travel(garil));
    assert((reads==std::vector<std::string>{"FLD_A5.FNT","FLD_A5.PRG"}));
    std::vector<std::uint8_t> out{99};
    assert(!prepared.take(1,"FLD_A3.PRG",out)&&out[0]==99);
    assert(prepared.take(1,"fld_a5.fnt",out)&&out.size()==16&&out[0]==1);
    assert(!prepared.take(1,"FLD_A5.FNT",out));
    assert(prepared.take(1,"FLD_A5.PRG",out)&&out[0]==2);
    assert(!prepared.take(1,"FLD_A5.PRG",out));
    // Town preparation uses the first signed byte in the two-byte pair,
    // not the WORLD selector directly as a 16-byte overlay-table index.
    memory.at(0x2166E4-0x200000+5*2)=7;
    memory.at(0x2166E4-0x200000+5*2+1)=11;
    put(0x2165D8+7*16,0x210040);text(0x210040,"TWN_CAMP.PRG");
    for(s8 type:{1,2,3}){
        reads.clear();assert(prepare_map_travel({0,0x52,5,type}));
        assert((reads==std::vector<std::string>{"TWN_CAMP.PRG"}));
        assert(prepared.take(1,"TWN_CAMP.PRG",out));
    }
    for(s8 selector:{-1,0}){
        reads.clear();assert(prepare_map_travel({0,0x56,selector,0}));assert(reads.empty());
    }
    // Optional native null filenames are skipped, keeping the authored order.
    put(0x208C58+3*20+8,0);reads.clear();assert(prepare_map_travel(garil));
    assert((reads==std::vector<std::string>{"FLD_A5.PRG"}));
    assert(!prepared.take(2,"FLD_A5.PRG",out));
    assert(!prepared.take(1,"FLD_A5.PRG",out)); // disc change invalidates retained bytes
    put(0x208C58+3*20+8,0x210000);
    failed="FLD_A5.PRG";assert(!prepare_map_travel(garil));
    assert(!prepared.take(1,"FLD_A5.FNT",out)); // no partially prepared transaction
    failed.clear();assert(prepare_map_travel(garil));prepared.clear();
    assert(!prepared.take(1,"FLD_A5.PRG",out));
    reads.clear();assert(!prepare_map_travel({0,-1,3,0}));assert(reads.empty());
    assert(!prepare_map_travel({0,0x56,3,4}));assert(reads.empty());
    puts("PASS native WORLD preparation selection/order, field/town tables, one-shot bytes, disc identity, failure atomicity and release; no hardware claim");
}

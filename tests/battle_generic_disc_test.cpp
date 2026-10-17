#include "lagi/battle_startup.h"
#include <array>
#include <vector>
#include <string>
#include <cstdio>
#include <cassert>
#include <algorithm>
#include <stdexcept>
using u8=std::uint8_t;using u16=std::uint16_t;using u32=std::uint32_t;
using s16=std::int16_t;using s32=std::int32_t;using fixedPoint=s32;
using quadColor=std::array<u16,4>;
struct sVec3_FP;
static FILE* disc=nullptr;
static std::vector<u8> extent(u32 lba,u32 size){
    if(size>0x1000000)throw std::runtime_error("Unexpected extent size");
    std::vector<u8> bytes(size);
    for(size_t at=0;at<size;at+=2048){
        const auto count=std::min<size_t>(2048,size-at);
        if(_fseeki64(disc,(std::uint64_t(lba)+at/2048)*2352+16,SEEK_SET) ||
            fread(bytes.data()+at,1,count,disc)!=count)throw std::runtime_error("Disc read failed");
    }
    return bytes;
}
static u32 little(const u8* p){return u32(p[0])|u32(p[1])<<8|u32(p[2])<<16|u32(p[3])<<24;}
static std::vector<u8> readProgram(const char* filename){
    const auto pvd=extent(16,2048);
    if(std::string(reinterpret_cast<const char*>(pvd.data()+1),5)!="CD001")throw std::runtime_error("Expected MODE1/2352 ISO9660");
    const auto root=extent(little(pvd.data()+158),little(pvd.data()+166));
    for(size_t at=0;at<root.size();){
        const auto size=root[at];
        if(!size){at=(at/2048+1)*2048;continue;}
        if(size<34 || at+size>root.size() || 33u+root[at+32]>size)throw std::runtime_error("Malformed directory");
        std::string name(reinterpret_cast<const char*>(root.data()+at+33),root[at+32]);
        name=name.substr(0,name.find(';'));
        if(name==filename)return extent(little(root.data()+at+2),little(root.data()+at+10));
        at+=size;
    }
    throw std::runtime_error("BTL_A3.PRG not found");
}
struct sSaturnMemoryFile;
struct sSaturnPtr{
    sSaturnMemoryFile* file;u32 address;
    sSaturnPtr operator+(u32 n)const{return {file,address+n};}
    sSaturnPtr& operator+=(u32 n){address+=n;return *this;}
};
struct sSaturnMemoryFile{
    std::vector<u8> storage;u8* m_data;u32 m_dataSize;u32 m_base=0x06054000;
    explicit sSaturnMemoryFile(const char* name):storage(readProgram(name)),m_data(storage.data()),m_dataSize(static_cast<u32>(storage.size())){}
    sSaturnPtr getSaturnPtr(u32 address){return {this,address};}
};
static unsigned reads=0;static u32 highest=0;
static u32 read(sSaturnPtr p,unsigned count){
    if(p.address<p.file->m_base || std::uint64_t(p.address-p.file->m_base)+count>p.file->m_dataSize)
        throw std::runtime_error("Native generic table read outside BTL_A3.PRG");
    ++reads;highest=std::max(highest,p.address+count);u32 value=0;
    for(unsigned i=0;i<count;++i)value=value<<8|p.file->m_data[p.address-p.file->m_base+i];
    return value;
}
u8 readSaturnU8(sSaturnPtr p){return static_cast<u8>(read(p,1));}
u16 readSaturnU16(sSaturnPtr p){return static_cast<u16>(read(p,2));}
s16 readSaturnS16(sSaturnPtr p){return static_cast<s16>(read(p,2));}
u32 readSaturnU32(sSaturnPtr p){return read(p,4);}
fixedPoint readSaturnFP(sSaturnPtr p){return static_cast<s32>(read(p,4));}
sSaturnPtr readSaturnEA(sSaturnPtr p){return {p.file,read(p,4)};}
#include "battle/battleGenericData.h"
#include "quad.inc"
#include "generic.inc"
int main(int argc,char** argv){
    if(argc!=2 || fopen_s(&disc,argv[1],"rb") || !disc)return 2;
    try{
        BTL_GenericData::staticInit();
        if(!g_BTL_GenericData || !reads || g_BTL_GenericData->m_dataSize<lagi::battle_generic_minimum_bytes)throw std::runtime_error("Generic data did not initialize");
        if(BTL_GenericData::m_0x60abef4_animatedQuads.size()!=9 || BTL_GenericData::m_0x60b0658_animatedQuad.empty())throw std::runtime_error("Missing native animation tables");
        std::printf("PASS: real BTL_A3.PRG bytes=%u; actual generic constructor/shared quad reader completed %u bounded reads through %08X; laser nodes=%d,%d,%d; animation groups=%zu. Read-only host data test, not battle execution.\n",
            g_BTL_GenericData->m_dataSize,reads,highest,BTL_GenericData::mLaserData.m20_numLaserNodes,
            BTL_GenericData::mBoltLaserData0.m20_numLaserNodes,BTL_GenericData::mBoltLaserData1.m20_numLaserNodes,
            BTL_GenericData::m_0x60abef4_animatedQuads.size());
        delete g_BTL_GenericData;fclose(disc);return 0;
    }catch(const std::exception& error){std::fprintf(stderr,"FAIL: %s\n",error.what());fclose(disc);return 1;}
}

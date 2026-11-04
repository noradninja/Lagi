#include "lagi/rbg_bank_access.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <map>
#include <set>
#include <string>
#include <vector>
using Bytes=std::vector<std::uint8_t>;
static unsigned le32(const std::uint8_t* p) {return p[0]|unsigned(p[1])<<8|unsigned(p[2])<<16|unsigned(p[3])<<24;}
static unsigned be16(const std::uint8_t* p) {return unsigned(p[0])<<8|p[1];}
static unsigned be32(const std::uint8_t* p) {return be16(p)<<16|be16(p+2);}
static Bytes read(std::ifstream& disc,unsigned lba,unsigned size) {
    Bytes bytes(size);
    for(unsigned offset=0;offset<size;offset+=2048) {
        const unsigned count=std::min(2048u,size-offset);
        disc.seekg(std::uint64_t(lba+offset/2048)*2352+16);
        disc.read(reinterpret_cast<char*>(bytes.data()+offset),count);
        assert(disc.good());
    }
    return bytes;
}
int main(int argc,char** argv) {
    assert(argc==2);
    std::ifstream disc(argv[1],std::ios::binary);assert(disc);
    const auto pvd=read(disc,16,2048);
    assert(std::string(reinterpret_cast<const char*>(pvd.data()+1),5)=="CD001");
    std::map<std::string,std::pair<unsigned,unsigned>> files;
    std::vector<std::pair<unsigned,unsigned>> pending{{le32(pvd.data()+158),le32(pvd.data()+166)}};
    std::set<unsigned> visited;
    while(!pending.empty()) {
        const auto [lba,size]=pending.back();pending.pop_back();
        if(!visited.insert(lba).second) continue;
        const auto dir=read(disc,lba,size);
        for(unsigned i=0;i<dir.size();) {
            const unsigned length=dir[i];
            if(!length) {i=(i/2048+1)*2048;continue;}
            assert(length>=34 && i+length<=dir.size());
            const auto* record=dir.data()+i;
            if(record[32] && record[33]>1) {
                const auto entry=std::pair{le32(record+2),le32(record+10)};
                if(record[25]&2) pending.push_back(entry);
                else {
                    std::string name(reinterpret_cast<const char*>(record+33),record[32]);
                    name=name.substr(0,name.find(';'));files.emplace(name,entry);
                }
            }
            i+=length;
        }
    }
    auto file=[&](const char* name) {assert(files.contains(name));const auto [lba,size]=files.at(name);return read(disc,lba,size);};
    const auto world=file("WORLD.PRG"),characters=file("FRS_WLD.SCB"),names=file("FRS_WLD.PNB");
    assert(world.size()>0x5c00 && characters.size()<=0x20000 && names.size()<=0x4000);
    Bytes vram(0x80000),cram(0x1000);
    std::copy(characters.begin(),characters.end(),vram.begin()+0x40000);
    std::copy(names.begin(),names.end(),vram.begin()+0x60000);
    for(const auto [src,dst,count]:std::array<std::array<unsigned,3>,4>{{
        {0x4d1c,0x400,0x200},{0x4f1c,0x600,0x200},{0x511c,0,0x20},{0x513c,0x200,0x200}}})
        std::copy_n(world.data()+src,count,cram.data()+dst);
    // WORLD: CHCN=0, CHSZ=1, PNB=1, CNSM=0, SCN=8. Count authored
    // texels, not projected raster output. Rotation/windows remain untested.
    unsigned oldAllowed=0,newAllowed=0,visible=0;std::set<unsigned> patterns;
    for(unsigned plane=0;plane<16;++plane) {
        const unsigned ea=be32(world.data()+0x53d0+plane*4);
        const unsigned base=ea>=0x25e00000?ea-0x25e00000:ea;
        assert(base>=0x60000 && base+2048<=0x60000+names.size());
        for(unsigned tile=0;tile<1024;++tile) {
            const unsigned name=be16(vram.data()+base+tile*2);
            if(!patterns.insert(name).second) continue;
            const unsigned address=(((name&0x3ff)<<2)|(8u<<10))*32u;
            assert(address>=0x40000 && address+128<=0x40000+characters.size());
            oldAllowed+=lagi::rbgBankReadable(0x1380,address,3);
            newAllowed+=lagi::rbgBankReadable(0x13b4,address,3);
            for(unsigned byte=0;byte<128;++byte) for(unsigned shift:{0u,4u}) {
                const unsigned dot=(vram[address+byte]>>shift)&15;
                const unsigned index=((name&0xf000)>>8)|dot;
                if(dot && (be16(cram.data()+index*2)&0x7fff)) ++visible;
            }
        }
    }
    assert(!oldAllowed && newAllowed==patterns.size() && visible);
    std::printf("PASS original WORLD 4bpp terrain assets: patterns=%zu coloredTexels=%u oldBankReadable=%u authoredBankReadable=%u; not camera/GPU validation\n",patterns.size(),visible,oldAllowed,newAllowed);
}

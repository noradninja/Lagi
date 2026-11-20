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
#include <functional>
#include <cmath>
using std::floor;using std::fmod;
struct float4 {float x,y,z,w;float4(float a=0,float b=0,float c=0,float d=0):x(a),y(b),z(c),w(d){}};
using sampler2D=const std::uint8_t*;
static float frac(float a) {return a-std::floor(a);}
static float readByte(sampler2D raw,float a) {return raw[unsigned(a)];}
static float readRbgAlignedBE16(sampler2D raw,float a) {const auto* p=raw+unsigned(a);return unsigned(p[0])*256u+p[1];}
// Compare native color words, not host approximations of GPU RGB channels.
static float4 rgb555(float word) {return {word,0,0,1};}
#include "world_terrain_shader.inc"
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
    const auto models=file("WORLDMAP.MCB");
    const unsigned groundList=be32(world.data()+0x481c)-0x06054000;
    const unsigned groundCount=be32(world.data()+0x4820);
    assert(groundCount && groundCount<world.size()/4 && groundList+groundCount*4<=world.size());
    unsigned meshNodes=0,meshModels=0,meshQuads=0;
    for(unsigned entry=0;entry<groundCount;++entry) {
        const auto* pair=world.data()+groundList+entry*4;
        const int hierarchySlot=std::int16_t(be16(pair)),poseSlot=std::int16_t(be16(pair+2));
        assert(hierarchySlot>=0&&poseSlot>=0&&unsigned(hierarchySlot)+4<=models.size()&&unsigned(poseSlot)+4<=models.size());
        const unsigned root=be32(models.data()+hierarchySlot),pose=be32(models.data()+poseSlot);
        assert(root&&pose);std::set<unsigned> nodes;
        std::function<void(unsigned)> walk=[&](unsigned node) {
            assert(node+12<=models.size()&&nodes.insert(node).second);
            ++meshNodes;
            const unsigned model=be32(models.data()+node);
            if(model) {
                assert(model+12<=models.size());++meshModels;
                const unsigned vertices=be32(models.data()+model+4),vertexData=be32(models.data()+model+8);
                assert(vertices && vertexData+vertices*6<=models.size());
                unsigned quad=model+12;
                while(true) {
                    assert(quad+8<=models.size());
                    if(!be32(models.data()+quad)&&!be32(models.data()+quad+4))break;
                    assert(quad+20<=models.size());
                    for(unsigned corner=0;corner<4;++corner)assert(be16(models.data()+quad+corner*2)<vertices);
                    const unsigned mode=(be16(models.data()+quad+8)>>8)&3;
                    quad+=20+std::array<unsigned,4>{0,8,48,24}[mode];++meshQuads;
                }
            }
            for(unsigned link:{4u,8u}) {const unsigned next=be32(models.data()+node+link);if(next)walk(next);}
        };
        walk(root);assert(pose+nodes.size()*36<=models.size());
    }
    assert(meshModels&&meshQuads);
    std::printf("PASS original WORLD base mesh references: entries=%u bones=%u modelSubmissions=%u quads=%u; not live frustum visibility\n",groundCount,meshNodes,meshModels,meshQuads);
    assert(world.size()>0x5c00 && characters.size()<=0x20000 && names.size()<=0x4000);
    Bytes vram(0x80000),cram(0x1000);
    std::copy(characters.begin(),characters.end(),vram.begin()+0x40000);
    std::copy(names.begin(),names.end(),vram.begin()+0x60000);
    for(const auto [src,dst,count]:std::array<std::array<unsigned,3>,4>{{
        {0x4d1c,0x400,0x200},{0x4f1c,0x600,0x200},{0x511c,0,0x20},{0x513c,0x200,0x200}}})
        std::copy_n(world.data()+src,count,cram.data()+dst);
    // WORLD: CHCN=0, CHSZ=1, PNB=1, CNSM=0, SCN=8. Count authored
    // texels, not projected raster output. Rotation/windows remain untested.
    Bytes raw=vram;raw.insert(raw.end(),cram.begin(),cram.end());
    std::array<float4,4> planes;
    for(unsigned group=0;group<4;++group) {
        float values[4];
        for(unsigned lane=0;lane<4;++lane) {
            const unsigned ea=be32(world.data()+0x53d0+(group*4+lane)*4);
            values[lane]=float(ea>=0x25e00000?ea-0x25e00000:ea);
        }
        planes[group]={values[0],values[1],values[2],values[3]};
    }
    float nameBanks[4],characterBanks[4];
    lagi::rbgBankPermissions(0x13b4,2,nameBanks);lagi::rbgBankPermissions(0x13b4,3,characterBanks);
    const float4 nb{nameBanks[0],nameBanks[1],nameBanks[2],nameBanks[3]};
    const float4 cb{characterBanks[0],characterBanks[1],characterBanks[2],characterBanks[3]};
    unsigned shaderChecks=0;
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
            for(unsigned y=0;y<16;++y) for(unsigned x=0;x<16;++x) {
                const unsigned sx=(name&0x400)?15-x:x,sy=(name&0x800)?15-y:y;
                const unsigned offset=((sy/8)*2+sx/8)*32+((sy%8)*8+sx%8)/2;
                const unsigned dot=(vram[address+offset]>>((sx&1)?0:4))&15;
                const unsigned index=((name&0xf000)>>8)|dot;
                const unsigned word=dot?be16(cram.data()+index*2):0;
                const auto actual=sampleRbg0Cell(raw.data(),float((plane%4)*512+(tile%32)*16+x),
                    float((plane/4)*512+(tile/32)*16+y),{0x100,0x8008,0,0},0,0,nb,cb,
                    planes[0],planes[1],planes[2],planes[3]);
                assert(actual.x==word && actual.w==(dot?1.f:0.f));++shaderChecks;
            }
            for(unsigned byte=0;byte<128;++byte) for(unsigned shift:{0u,4u}) {
                const unsigned dot=(vram[address+byte]>>shift)&15;
                const unsigned index=((name&0xf000)>>8)|dot;
                if(dot && (be16(cram.data()+index*2)&0x7fff)) ++visible;
            }
        }
    }
    assert(!oldAllowed && newAllowed==patterns.size() && visible);
    std::printf("PASS original WORLD 4bpp terrain: patterns=%zu coloredTexels=%u oldBankReadable=%u authoredBankReadable=%u shaderCellChecks=%u; not camera/GPU validation\n",patterns.size(),visible,oldAllowed,newAllowed,shaderChecks);
}

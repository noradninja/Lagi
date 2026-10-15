#include "lagi/azel_map_menu.h"
#include <cassert>
#include <cstdio>
#include <string>
static std::array<std::uint8_t,0x80000> vram,baseline;
static std::vector<std::uint8_t> graphics,patterns;
static unsigned failure=0,loadCalls=0;
static std::uint32_t getFileSize(const char* name)
{
    if(!std::strcmp(name,"MAP.SCB"))return static_cast<std::uint32_t>(graphics.size());
    if(!std::strcmp(name,"MAP.PNB"))return static_cast<std::uint32_t>(patterns.size());
    return 0;
}
static std::uint8_t* getVdp2Vram(std::uint32_t offset){assert(offset<vram.size());return vram.data()+offset;}
static int loadFile2(const char* name,std::uint8_t* destination,int flags)
{
    assert(flags==0);++loadCalls;
    const bool scb=!std::strcmp(name,"MAP.SCB");
    const auto& file=scb?graphics:patterns;
    const bool fail=failure==(scb?1u:2u);
    // Native load failure may occur after touching its destination.
    std::memcpy(destination,file.data(),fail?file.size()/2:file.size());
    return fail?-1:0;
}
#include "map_owner_loader_production.inc"
#include "map_owner_production.inc"
static void reset(unsigned graphicsBytes,unsigned patternBytes)
{
    for(unsigned i=0;i<vram.size();++i)vram[i]=static_cast<std::uint8_t>(i*37+19);
    baseline=vram;graphics.assign(graphicsBytes,0xA5);patterns.assign(patternBytes,0x5A);
    failure=0;loadCalls=0;
}
int main()
{
    LagiAzelMapDescriptor descriptor;
    descriptor.mapFile="MAP.SCB";descriptor.patternFile="MAP.PNB";
    unsigned cases=0;
    for(unsigned graphicsBytes : {0x1000u,0x4000u,0x4001u,0x7000u,0x8000u,0x17000u})
    for(unsigned patternBytes : {0x400u,0x800u,0x1000u}) {
        reset(graphicsBytes,patternBytes);
        LagiAzelMapResources owner;
        assert(owner.load(descriptor) && owner.ready() && loadCalls==2);
        const unsigned destination=graphicsBytes>0x4000?0x69000:0x6C000;
        assert(owner.graphicsVramOffset()==destination);
        assert(std::memcmp(vram.data()+destination,graphics.data(),graphicsBytes)==0);
        assert(std::memcmp(owner.patterns().data(),patterns.data(),patternBytes)==0);
        if(patternBytes<=0x800)for(unsigned i=0x800;i<0x1000;i+=2) {
            assert(owner.patterns()[i]==2 && owner.patterns()[i+1]==0x38);
        }
        const auto current=vram;
        assert(!owner.load(descriptor) && owner.ready() && vram==current && loadCalls==2);
        // Future native view conversion writes both planes independently.
        std::memset(vram.data()+0x70000,0xCC,0x1000);
        // An unrelated live layer write must not be rolled back by this owner.
        vram[0x100]^=0xFF;baseline[0x100]^=0xFF;
        owner.restore();
        assert(!owner.ready() && owner.graphicsVramOffset()==0 && vram==baseline);
        vram[0x70000]^=0xFF;
        owner.restore();assert(vram[0x70000]!=baseline[0x70000]); // Idempotent.
        ++cases;
    }
    for(unsigned fail : {1u,2u})for(unsigned graphicsBytes : {0x4000u,0x8000u}) {
        reset(graphicsBytes,0x400);failure=fail;
        LagiAzelMapResources owner;
        const auto oldPatterns=owner.patterns();
        assert(!owner.load(descriptor) && !owner.ready() && owner.graphicsVramOffset()==0);
        assert(vram==baseline && owner.patterns()==oldPatterns && loadCalls==fail);
        failure=0;assert(owner.load(descriptor) && owner.ready());
        owner.restore();assert(vram==baseline);++cases;
    }
    reset(0x8000,0x1000);
    {
        LagiAzelMapResources owner;assert(owner.load(descriptor));
        std::memset(vram.data()+0x70000,0xDD,0x1000);
        // Simulate forced teardown that bypasses the parent's explicit restore.
    }
    assert(vram==baseline);
    for(auto sizes : {std::array<unsigned,2>{0,0x800},{0x17001,0x800},{0x4000,0},{0x4000,0x1001}}) {
        reset(sizes[0],sizes[1]);LagiAzelMapResources owner;
        assert(!owner.load(descriptor) && !owner.ready() && loadCalls==0 && vram==baseline);
    }
    reset(0x4000,0x800);LagiAzelMapResources owner;
    descriptor.patternFile=nullptr;
    assert(!owner.load(descriptor) && loadCalls==0 && vram==baseline);
    std::printf("PASS: %u production native-loader ownership/overlap/partial-failure cases, forced teardown, exact-span restoration and invalid/reentrant guards (host mocks)\n",cases);
}

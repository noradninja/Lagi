#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <vector>
using u8=uint8_t;using u16=uint16_t;using s16=int16_t;
using u32=uint32_t;using s32=int32_t;using fixedPoint=s32;
using sVec3_FP=std::array<s32,3>;
struct sSaturnPtr {u32 address=0;sSaturnPtr operator+(u32 n)const{return {address+n};}};
struct File {sSaturnPtr getSaturnPtr(u32 p){return {p};}} file;
auto* g_BTL_A3=&file;
std::vector<u8> bytes;
u32 read(sSaturnPtr p,unsigned size){
    assert(p.address>=0x06054000&&uint64_t(p.address-0x06054000)+size<=bytes.size());
    u32 value=0;for(unsigned i=0;i<size;++i)value=(value<<8)|bytes[p.address-0x06054000+i];return value;
}
u8 readSaturnU8(sSaturnPtr p){return u8(read(p,1));}
u16 readSaturnU16(sSaturnPtr p){return u16(read(p,2));}
s16 readSaturnS16(sSaturnPtr p){return s16(read(p,2));}
fixedPoint readSaturnFP(sSaturnPtr p){return s32(read(p,4));}
sSaturnPtr readSaturnEA(sSaturnPtr p){return {read(p,4)};}
unsigned draws=0,emissions=0;
u32 randomNumber(){++draws;return 0xABCD8001u;}
#include "lagi_battle_projectile_descriptor.inc"
int main(int argc,char** argv){
    assert(argc==2);FILE* disc=nullptr;fopen_s(&disc,argv[1],"rb");assert(disc);
    bytes.resize(385044);
    for(size_t at=0;at<bytes.size();at+=2048){
        assert(!_fseeki64(disc,(uint64_t(258144)+at/2048)*2352+16,SEEK_SET));
        const size_t size=std::min<size_t>(2048,bytes.size()-at);
        assert(fread(bytes.data()+at,1,size,disc)==size);
    }
    fclose(disc);
    const u32 anchors[][2]={
        {0x06056998,0xAE61},{0x0605699A,0xE501},
        {0x0605699C,0xAE5F},{0x0605699E,0xE500},
        {0x060569A0,0xAF25},{0x060569A2,0xE501},
        {0x060569A4,0xAF23},{0x060569A6,0xE500},
        {0x06056688,0xE303},{0x06056694,0xE014},
        {0x06056818,0xE010},{0x0605687A,0x7FFF},
        {0x0605687C,0xC001},{0x060569F6,0x03FF}
    };
    for(const auto& a:anchors)assert(read({a[0]},2)==a[1]);
    const auto original=lagiReadBattleProjectileDescriptor({0x060A7B90});
    assert(original.slot==read({0x060A7B90},1));
    assert(original.hierarchy==read({0x060A7B92},2));
    assert(original.travel==s16(read({0x060A7B9C},2)));
    assert(original.lifetime==s16(read({0x060A7B9E},2)));
    assert(original.hit.address==read({0x060A7BB4},4));
    const sVec3_FP source={11,22,33},dragon={100,200,300};
    for(bool alternate:{false,true})for(bool strong:{false,true}){
        draws=0;emissions=0;
        const unsigned exact=strong?3:2;
        const unsigned count=alternate?(strong?16:8):(strong?20:8);
        lagiBuildUrchinProjectilePattern(source,dragon,alternate,strong,
            [&](const auto& from,const auto& target,const auto& d){
                assert(from==source&&d.slot==12&&d.hierarchy==12&&d.pose==0xDC);
                assert(d.animation==0&&d.quad.address==0&&d.muzzle.address==0);
                assert(d.damage==(alternate?9:20)&&d.lifetime==(alternate?60:30));
                if(alternate){
                    assert(draws==(emissions+1)*6);
                    assert((target==sVec3_FP{-16282,-16182,-16082}));
                    assert(d.horizontalAcceleration==-510&&d.verticalAcceleration==-510);
                    assert(d.travel==31&&d.hit.address==0x060A8680);
                    assert(d.trail.address==0x060A87B4&&d.trailMask==1&&d.trailSlot==12&&d.hitSlot==12);
                }else{
                    assert(draws==emissions+1+(emissions<exact?0:(emissions+1-exact)*3));
                    assert(target==(emissions<exact?dragon:sVec3_FP{-32666,-32566,-32466}));
                    assert(d.travel==10&&d.horizontalAcceleration==0&&d.verticalAcceleration==-0x160);
                    assert(d.hit.address==0x060AE668&&d.hitSlot==0&&d.trail.address==0);
                }
                ++emissions;
            });
        assert(emissions==count);
    }
    std::puts("Native A3 projectile descriptor / four attack pattern modes / RNG ordering PASS (not runtime dispatch)");
}

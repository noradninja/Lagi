#include <array>
#include <cstdint>
#include <cassert>
#include <cstdio>
using u32=std::uint32_t;using u16=std::uint16_t;using s32=std::int32_t;
struct s_nameEntrySpriteData{u16 m0_CMDCTRL,m2_CMDPMOD,m4_CMDSRCAbase,m6_CMDSIZE,m8_CMDCOLR,mA_pad;s32 mC_widthFP,m10_heightFP,m14_xOffsetFP,m18_yOffsetFP;};
static std::array<unsigned char,112> data;
struct Overlay {const unsigned char* getSaturnPtr(unsigned address){assert(address>=0x210768 && address<0x2107d8);return data.data()+address-0x210768;}} overlay;
auto* gCommonFile=&overlay;
u16 readSaturnU16(const unsigned char* p){return u16(p[0])*256+p[1];}
s32 readSaturnS32(const unsigned char* p){return s32(u32(readSaturnU16(p))*65536+readSaturnU16(p+2));}
#include "name_cursor_fixture.inc"
int main(){
    for(unsigned i=0;i<data.size();++i)data[i]=static_cast<unsigned char>(i*7+3);
    for(unsigned i=0;i<4;++i){
        const auto sprite=nameEntryNativeCursor(i);const auto* p=data.data()+i*28;
        assert(sprite.m0_CMDCTRL==readSaturnU16(p+2));
        assert(sprite.m2_CMDPMOD==readSaturnU16(p+4));
        assert(sprite.m4_CMDSRCAbase==readSaturnU16(p+6));
        assert(sprite.m6_CMDSIZE==readSaturnU16(p+8));
        assert(sprite.m8_CMDCOLR==readSaturnU16(p+10));
        assert(sprite.mC_widthFP==readSaturnS32(p+12));
        assert(sprite.m10_heightFP==readSaturnS32(p+16));
        assert(sprite.m14_xOffsetFP==readSaturnS32(p+20));
        assert(sprite.m18_yOffsetFP==readSaturnS32(p+24));
    }
    assert(nameEntryNativeCursor(3).m6_CMDSIZE!=nameEntryNativeCursor(1).m6_CMDSIZE);
    std::puts("PASS actual native cursor reader: all four live records, header offsets, signed geometry, entry 3 not masked");
}

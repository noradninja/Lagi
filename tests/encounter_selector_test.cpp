#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <vector>
using u8=std::uint8_t;using u16=std::uint16_t;using u32=std::uint32_t;using s8=std::int8_t;
static std::vector<u8> common(0x9000);
struct sSaturnPtr {u32 m_offset; sSaturnPtr operator+(unsigned n)const{return {m_offset+n};}};
static u8 byte(sSaturnPtr p){assert(p.m_offset>=0x200000 && p.m_offset<0x209000);return common[p.m_offset-0x200000];}
static u16 readSaturnU16(sSaturnPtr p){return (u16(byte(p))<<8)|byte(p+1);}
static s8 readSaturnS8(sSaturnPtr p){return s8(byte(p));}
static sSaturnPtr readSaturnEA(sSaturnPtr p){return {(u32(readSaturnU16(p))<<16)|readSaturnU16(p+2)};}
struct Common {sSaturnPtr getSaturnPtr(u32 address){return {address};}} commonFile;
static Common* gCommonFile=&commonFile;
struct Field {int m2C_currentFieldIndex=2;} field;
static Field* fieldTaskPtr=&field;
struct State {u8 bitField[630]{};
#include "get_bit.inc"
} mainGameState;
struct sEncouterTask {sSaturnPtr m18{};s8 mC=-1,m0_status=0;};
namespace lagi::platform::logging {template<class... T> void writef(const char*,T...) {}}
static void initialize(sEncouterTask* pThis){
#include "selector_init.inc"
}
static u8 discByte(FILE* f,u32 lba,u32 offset){
    const auto position=std::int64_t(lba+offset/2048)*2352+16+offset%2048;
    assert(_fseeki64(f,position,SEEK_SET)==0);const auto value=std::fgetc(f);assert(value!=EOF);return u8(value);
}
static u16 mainWord(FILE* f,u32 ea){return (u16(discByte(f,58,ea-0x6006000))<<8)|discByte(f,58,ea+1-0x6006000);}
// Execute the original checked five-instruction condition, including TST's
// zero sense. This is deliberately independent of C++ getBit/?: selection.
static unsigned nativeOffset(const std::array<u16,5>& code,u8 masked){
    bool t=false;unsigned r0=0;
    for(const auto op:code)switch(op){
        case 0x2228:t=(masked==0);break; // TST R2,R2
        case 0x0029:r0=t;break;         // MOVT R0
        case 0xca01:r0^=1;break;       // XOR #1,R0
        case 0x4008:r0<<=2;break;      // SHLL2 R0
        case 0x4000:r0<<=1;break;      // SHLL R0
        default:assert(false);
    }
    return r0;
}
int main(int argc,char** argv){
    assert(argc==2);FILE* f=nullptr;assert(fopen_s(&f,argv[1],"rb")==0 && f);
    for(unsigned i=0;i<common.size();++i)common[i]=discByte(f,182,i);
    std::array<u16,5> code{};for(unsigned i=0;i<5;++i)code[i]=mainWord(f,0x600c74a+i*2);
    const std::array<u16,5> expected={0x2228,0x0029,0xca01,0x4008,0x4000};assert(code==expected);
    // Complete selector initialization stream and data literals. Branch
    // instructions above the selector enter this block only for status zero.
    const u16 prefix[]={0xd37e,0x430b,0x0009,0xd47d,0xe12c,0x001d,0x6303,0x4008,
        0x303c,0x4008,0x600f,0x340c,0x8541,0x6503,0x635f,0xd078,0x4309,0x4301,
        0x023c,0xd377,0x605f,0xc907,0x033c,0x2239};
    for(unsigned i=0;i<std::size(prefix);++i)assert(mainWord(f,0x600c71a+i*2)==prefix[i]);
    const u16 suffix[]={0x6343,0x7304,0x303c,0x1e06,0x8441,0x80ec};
    for(unsigned i=0;i<std::size(suffix);++i)assert(mainWord(f,0x600c754+i*2)==suffix[i]);
    assert(mainWord(f,0x600c918)==0x0020 && mainWord(f,0x600c91a)==0x8710);
    assert(mainWord(f,0x600c920)==0x0604 && mainWord(f,0x600c922)==0x2f14);
    for(unsigned bit=0;bit<8;++bit)assert(discByte(f,58,0x6042f14+bit-0x6006000)==(0x80>>bit));
    std::fclose(f);
    const sSaturnPtr record{0x208738};assert(readSaturnU16(record+2)==87);
    for(unsigned set=0;set<2;++set){
        mainGameState.bitField[87/8]=set?1:0;
        sEncouterTask task;initialize(&task);
        assert(task.m18.m_offset==record.m_offset+4+nativeOffset(code,set?1:0));
        assert(task.m0_status==1 && task.mC==0);
        assert(readSaturnEA(task.m18).m_offset==(set?0x2076a0:0x207624));
        assert(readSaturnU16(task.m18+4)==(set?150:120));
        assert(readSaturnS8(task.m18+6)==(set?120:90));
        assert(readSaturnS8(task.m18+7)==(set?20:10));
    }
    // Generic bit positions and nonzero unrelated bits: no hardcoded A3 bit.
    for(unsigned bit=0;bit<630*8;++bit)for(unsigned set=0;set<2;++set){
        common[record.m_offset-0x200000+2]=u8(bit>>8);
        common[record.m_offset-0x200000+3]=u8(bit);
        const u8 mask=u8(0x80>>(bit%8));
        mainGameState.bitField[bit/8]=u8((0x5a&~mask)|(set?mask:0));
        sEncouterTask task;initialize(&task);
        assert(task.m18.m_offset==record.m_offset+4+nativeOffset(code,set?mask:0));
    }
    std::puts("PASS original story-selector instructions/literals/masks; production initializer and getBit for both native A3 branches and 5040 generic bit positions");
}

#include <array>
#include <cstdint>
#include <cassert>
#include <cstdio>
#include <climits>
using s32=std::int32_t;using u32=std::uint32_t;
struct FP {s32 v=0;s32 asS32()const{return v;}};
using sVec3_FP=std::array<FP,3>;
std::array<unsigned char,0x22000> bytes{};
struct Ptr {u32 m_offset;Ptr operator+(u32 n)const{return {m_offset+n};}};
struct Common {unsigned char* m_data=bytes.data();u32 m_base=0x200000;std::size_t m_dataSize=bytes.size();Ptr getSaturnPtr(u32 a){return {a};}} common;
Common* gCommonFile=&common;
struct Field {int m2C_currentFieldIndex=1;} field;
Field* fieldPtr=&field;Field* getFieldTaskPtr(){return fieldPtr;}
unsigned char at(Ptr p){assert(p.m_offset>=0x200000 && p.m_offset-0x200000<bytes.size());return bytes[p.m_offset-0x200000];}
s32 readSaturnS8(Ptr p){return static_cast<std::int8_t>(at(p));}
s32 readSaturnS16(Ptr p){return static_cast<std::int16_t>((unsigned(at(p))<<8)|at(p+1));}
Ptr readSaturnEA(Ptr p){return {(u32(at(p))<<24)|(u32(at(p+1))<<16)|(u32(at(p+2))<<8)|at(p+3)};}
struct State {
 std::array<unsigned char,1024> saved{};unsigned writes=0;
 void setBit566(u32 n){assert(n>=0x95A && n<0x10D8);n=n<1000?n:n-566;saved[n/8]|=0x80>>(n%8);++writes;}
} mainGameState;
#include "../src/integration/lagi_field_map_exploration.inc"
void put(unsigned offset,u32 value,unsigned count=4){for(unsigned i=0;i<count;++i)bytes[offset+i]=value>>(8*(count-1-i));}
int main(){
 put(0xA388+4,0x20B000);put(0xB000,0x20B100);put(0xB100,0x20B200);
 bytes[0xB200]=0xF8;bytes[0xB201]=0xF8;bytes[0xB202]=16;bytes[0xB203]=16;
 put(0xB108,0x95A,2);
 unsigned cases=0;
 for(s32 x:{INT_MIN,-0x400001,-0x400000,-0x200001,-0x200000,-1,0,1,0x1FFFFF,0x200000,0x200001,INT_MAX})
 for(s32 z:{INT_MIN,-0x400001,-0x400000,-0x200001,-0x200000,-1,0,1,0x1FFFFF,0x200000,0x200001,INT_MAX}){
  mainGameState={};sVec3_FP pos{{{x},{123},{z}}};
  fieldTerrainCallback(0,&pos,nullptr);
  // Independent 64-bit SH2 DIVS/NEG/sign-adjust oracle.
  std::int64_t nx=x,nz=z==INT_MIN?INT_MIN:-std::int64_t(z);
  const auto cx=nx/0x200000+8-(x<0),cy=nz/0x200000+8-(z>0);
  const bool inside=cx>=0 && cy>=0 && cx<16 && cy<16;
  assert(mainGameState.writes==unsigned(inside));
  if(inside){const unsigned bit=unsigned(0x95A+cy*16+cx)-566;assert(mainGameState.saved[bit/8]==(0x80>>(bit%8)));}
  const auto saved=mainGameState.saved;fieldTerrainCallback(0,&pos,nullptr);assert(mainGameState.saved==saved);
  ++cases;
 }
 sVec3_FP origin{};
 for(unsigned base:{0u,0x959u,0x95Au,0x10D7u,0x10D8u,0xFFFFu}){
  bytes[0xB200]=bytes[0xB201]=0;put(0xB108,base,2);mainGameState={};
  fieldTerrainCallback(0,&origin,nullptr);assert(mainGameState.writes==unsigned(base>=0x95A && base<0x10D8));
 }
 const auto unchanged=mainGameState.saved;
 fieldTerrainCallback(-1,&origin,nullptr);fieldTerrainCallback(INT_MAX,&origin,nullptr);fieldTerrainCallback(0,nullptr,nullptr);
 field.m2C_currentFieldIndex=23;fieldTerrainCallback(0,&origin,nullptr);field.m2C_currentFieldIndex=1;
 common.m_dataSize=1;fieldTerrainCallback(0,&origin,nullptr);common.m_dataSize=bytes.size();
 put(0xB100,0x2FFFFF);fieldTerrainCallback(0,&origin,nullptr);
 put(0xB000,0);fieldTerrainCallback(0,&origin,nullptr);
 put(0xA388+4,0);fieldTerrainCallback(0,&origin,nullptr);
 assert(mainGameState.saved==unchanged);
 std::printf("PASS: production native exploration callback, %u signed coordinate pairs, exact boundaries, range guards, idempotent saved bits and malformed tables; no hardware validation\n",cases);
}

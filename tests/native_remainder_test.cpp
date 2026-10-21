#include <cstdint>
#include <cassert>
#include <cstdio>
#include <initializer_list>
using u32=std::uint32_t;
unsigned reports=0;
template<class... T>void PDS_Log(const char*,T...){++reports;}
#include "remainder_body.inc"
int main(){
 for(u32 divisor:{0u,1u,2u,3u,0x800u,0x7fffffffu,0xffffffffu})
  for(u32 dividend:{0u,1u,7u,0x80000000u,0xffffffffu}){
   const u32 expected=divisor?dividend%divisor:0;
   assert(performModulo(divisor,dividend)==expected);
   assert(performModulo2(divisor,dividend)==expected);
  }
 for(unsigned i=0;i<20;++i){assert(performModulo(0,i)==0);assert(performModulo2(0,i)==0);}
 assert(reports==16);
 puts("PASS: actual native remainder adapters, zero-divisor return and bounded reporting; not hardware validation");
}

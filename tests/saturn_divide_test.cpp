#include "lagi/saturn_divide.h"
#include <cassert>
#include <cstdio>
#include <initializer_list>
int main(){
 using lagi::saturn_mul_div;
 constexpr auto lo=INT32_MIN,hi=INT32_MAX;
 assert(saturn_mul_div(0,0,0)==hi);
 assert(saturn_mul_div(2,-3,0)==lo);
 assert(saturn_mul_div(-2,-3,0)==hi);
 assert(saturn_mul_div(lo,lo,1)==hi);
 assert(saturn_mul_div(lo,hi,1)==lo);
 assert(saturn_mul_div(lo,1,-1)==hi);
 assert(saturn_mul_div(lo,1,1)==lo);
 assert(saturn_mul_div(hi,1,1)==hi);
 for(auto divisor:{-65536,-100,-3,-1,1,3,100,65536})
  for(int a=-300;a<=300;++a)for(int b=-20;b<=20;++b)
   assert(saturn_mul_div(a,b,divisor)==std::int64_t(a)*b/divisor);
 // Lock-on projections crossing the camera plane: defined on both sides,
 // saturated at z=0 rather than a reconstructed engine assertion.
 assert(saturn_mul_div(176,65536,0)==hi);
 assert(saturn_mul_div(176,-65536,0)==lo);
 assert(saturn_mul_div(176,65536,1)==11534336);
 assert(saturn_mul_div(176,65536,-1)==-11534336);
 puts("PASS shared Saturn multiply/divide: signed truncation, zero denominator, overflow and camera-plane crossings; not hardware route validation");
}

#include <cstdint>
#include <cstdlib>
#include <cassert>
#include <array>
#include <cstdio>
using s32=std::int32_t;using u32=std::uint32_t;
struct fixedPoint {s32 m_value;fixedPoint(s32 v):m_value(v){}s32 asS32()const{return m_value;}};
static fixedPoint MTH_Mul(fixedPoint a,fixedPoint b){return s32((std::int64_t(a.m_value)*b.m_value)>>16);}
static fixedPoint FP_Div(s32 a,fixedPoint b){assert(b.m_value);return s32((std::int64_t(a)*65536)/b.m_value);}
static bool allocationFails=false;static unsigned live=0,allocations=0;
static void* allocateHeap(u32 bytes){++allocations;if(allocationFails)return nullptr;++live;return std::malloc(bytes);}
static void freeHeap(void* p){assert(live);--live;std::free(p);}
#include "../src/integration/lagi_field_spline_coefficients.inc"
int main(){
 std::array<s32,3> arc{0,32768,65536},src{0,65536,0},coeff{99,99,99};
 fieldSpline_precompute(3,arc.data(),src.data(),coeff.data());
 assert((coeff==std::array<s32,3>{0,-512,0}) && !live);
 src={65536,0,65536};fieldSpline_precompute(3,arc.data(),src.data(),coeff.data());
 assert((coeff==std::array<s32,3>{0,512,0}) && !live);
 // Affine data has zero second derivatives even with nonuniform intervals.
 for(s32 slope:{-16,-3,0,1,7,32})for(s32 offset:{-0x600000,0,0x5ff000}) {
  std::array<s32,6>a{0,4096,12288,28672,45056,65536},s{},c{};
  for(unsigned i=0;i<6;++i)s[i]=a[i]*slope+offset;
  fieldSpline_precompute(6,a.data(),s.data(),c.data());
  for(auto value:c)assert(!value);
  assert(!live);
 }
 // Translation must not affect derivatives on a curved, nonuniform path.
 std::array<s32,6>a{0,8192,24576,32768,49152,65536},s{0,0x8000,-0x4000,0x9000,0x1000,0x6000},c{},shifted{},other{};
 fieldSpline_precompute(6,a.data(),s.data(),c.data());
 for(unsigned i=0;i<6;++i)shifted[i]=s[i]+0x5ff000;
 fieldSpline_precompute(6,a.data(),shifted.data(),other.data());assert(c==other);
 assert(c.front()==0 && c.back()==0);
 coeff={99,99,99};allocationFails=true;
 fieldSpline_precompute(3,arc.data(),src.data(),coeff.data());
 assert((coeff==std::array<s32,3>{99,99,99}) && !live);
 std::printf("PASS production native spline coefficients: golden curvature, nonuniform affine/translation invariants, natural boundaries, allocation ownership/failure (%u calls); evaluator endpoint/device pending\n",allocations);
}

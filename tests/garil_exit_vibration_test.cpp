#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
using s32=std::int32_t;
struct fixedPoint {s32 value=0;fixedPoint(s32 v=0):value(v){}};
struct sVec3_FP {fixedPoint m0_X,m4_Y,m8_Z;};
template<class T>struct s_workAreaTemplate {std::array<std::uint64_t,9> hostTaskFields{};};
std::array<int,4> impulses{};int impulseCount=0;
void addCameraImpulse(void* owner,sVec3_FP* impulse){assert(!owner);impulses[impulseCount++%4]=impulse->m0_X.value;}
#include "production.inc"
int main(){
 sA5ExitTriggerVibrationTask child{};
 struct Parent {void* m2C_childTask;} parent{&child};
 auto* pThis=&parent;
 child.hostTaskFields.fill(0x123456789ABCDEF0ULL);
 child.m10_intensity=100;child.m18_phase=0;
 // This is the generated parent mode1 write, not a handwritten setter.
#include "set.inc"
 assert(child.m14_storedIntensity==0x1B4E81&&child.m10_intensity==100);
 for(auto word:child.hostTaskFields)assert(word==0x123456789ABCDEF0ULL);
 for(int i=0;i<4;++i)a5ExitTriggerVibration_Update(&child);
 assert((impulses==std::array<int,4>{100,-100,50,-50}));
 assert(child.m18_phase==0&&child.m10_intensity==0x1B4E81);
#include "clear.inc"
 assert(!child.m14_storedIntensity&&child.m10_intensity==0x1B4E81);
 for(int i=0;i<4;++i)a5ExitTriggerVibration_Update(&child);
 assert((impulses==std::array<int,4>{0x1B4E81,-0x1B4E81,0xDA740,-0xDA740}));
 assert(child.m10_intensity==0&&child.m18_phase==0);
 const auto oldCount=impulseCount;
 for(int i=0;i<4;++i)a5ExitTriggerVibration_Update(&child);
 assert(impulseCount==oldCount&&child.m18_phase==0);
 for(auto word:child.hostTaskFields)assert(word==0x123456789ABCDEF0ULL);
 puts("PASS generated Garil parent writes and production child update: stored/current distinction, four-phase delayed latch, stop boundary and host task fields preserved; device exit unverified");
}

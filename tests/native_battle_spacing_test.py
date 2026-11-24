from pathlib import Path
import subprocess,tempfile,sys
r=Path(__file__).resolve().parents[1]
f=r'''
#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include "lagi/native_battle_spacing.h"
struct F{int32_t value=0;F(int32_t v=0):value(v){}int32_t asS32()const{return value;}};
using V=std::array<F,3>;using Positions=std::array<V,6>;
uint32_t next(uint32_t& state){state=state*1664525u+1013904223u;return state;}
int32_t distance(const V& a,const V& b){double sum=0;for(unsigned axis=0;axis<3;++axis){const double d=double(a[axis].value)-b[axis].value;sum+=d*d;}return int32_t(std::sqrt(sum));}
void reference(Positions& p,int count,unsigned live,uint32_t& rng,unsigned& calls){
 auto random=[&](){++calls;return next(rng);};
 for(int i=0;i<count;++i)if(live&(1u<<i))for(int j=0;j<count;++j)if(live&(1u<<j)){
  auto& other=p[j];if(i==j){if(j<2){other[0].value+=(random()&8191)-4096;other[2].value+=(random()&8191)-4096;}}
  else{const auto dx=p[i][0].value-other[0].value;const auto dz=p[i][2].value-other[2].value;const auto d=distance(p[i],other);
   if(d<40960){other[0].value+=dx<0?4096:-4096;other[2].value+=dz<0?4096:-4096;}
   else if(d<204800){other[0].value+=(random()&8191)-4096;other[2].value+=(random()&8191)-4096;}
   else{other[0].value+=dx<0?-8192:8192;other[2].value+=dz<0?-16384:16384;}}
  if(other[0].value>=102400)other[0].value=102400;if(other[0].value<=-102400)other[0].value=-102400;
  if(other[2].value>=81920)other[2].value=81920;if(other[2].value<=-81920)other[2].value=-81920;
 }
}
int main(){uint32_t seed=0xABCDEF;for(unsigned sample=0;sample<24000;++sample){const int count=sample%2?3:6;const unsigned live=next(seed)&63;Positions production,expected;for(auto& point:production)for(auto& axis:point)axis=int32_t(next(seed)%600001)-300000;expected=production;uint32_t a=next(seed),b=a;unsigned ca=0,cb=0;lagi::stepNativeBattleSpacing(production,count,[&](int i){return live&(1u<<i);},[&](){++ca;return next(a);},distance);reference(expected,count,live,b,cb);for(unsigned i=0;i<6;++i)for(unsigned axis=0;axis<3;++axis)assert(production[i][axis].value==expected[i][axis].value);assert(a==b&&ca==cb);}
 puts("PASS shared native spacing: 24000 three/six-actor cases against independent ordered-pair reference, exact RNG order/count, liveness and bounds");}
'''
with tempfile.TemporaryDirectory(prefix='lagi-native-spacing-') as d:
 p=Path(d);(p/'test.cpp').write_text(f)
 subprocess.run(['cl','/nologo','/EHsc','/std:c++20','/I'+str(r/'include'),str(p/'test.cpp'),'/Fe:'+str(p/'test.exe'),'/Fo:'+str(p/'test.obj')],check=True)
 subprocess.run([str(p/'test.exe')],check=True)
s=(r/'build-vita/azel_upstream/battle_BTL_A5_BTL_A5_dual.cpp').read_text()
assert s.index('lagiInitGarilDualSingleLayout(pThis);')<s.index('lagiCreateGarilDualSingle(pThis,i)')
assert s.index('lagiUpdateGarilDualSpacing(pThis);')<s.index('formationUpdatePosition(&pThis->m2C_posBlock')
assert 'return dual->m_lagiSinglePositions[entity];' in s
data=Path(sys.argv[1]).read_bytes();base=0x6054000
for ea,value,n in [(0x605F992,0x1000,2),(0x605F994,0x1FFF,2),(0x605F996,0xF000,2),(0x605FAD0,0xA000,4),(0x605FAD4,0x32000,4),(0x605FBE4,0x19000,4),(0x605FBE8,0xFFFE7000,4),(0x605FBEC,0x14000,4),(0x605FBF0,0xFFFEC000,4),(0x605FACC,0x6035A5C,4)]:
 assert int.from_bytes(data[ea-base:ea-base+n],'big')==value,f'Original spacing contract changed at {ea:08X}'
print('PASS original dual spacing thresholds/clamps/RNG operands and generated layout ownership/order')

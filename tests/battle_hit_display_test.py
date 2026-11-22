"""Compile the generated shared hit producer; compare native A3/A5 zero branches."""
from pathlib import Path
import subprocess, sys, tempfile
r=Path(__file__).resolve().parents[1]
assert len(sys.argv)==2, 'Pass original extracted data directory'
data=Path(sys.argv[1])
for name,ea in [('BTL_A3.PRG',0x0605BB10),('BTL_A5_3.PRG',0x0605A658)]:
 b=(data/name).read_bytes(); i=ea-0x06054000
 # Read damage, test zero, set COLOR offset42 to3, branch to position copy.
 expected=[0xE022,0x03ED,0x2338,0x8B15,0xE203,0xE042,0xA028,0x0E25]
 assert [int.from_bytes(b[i+j:i+j+2],'big') for j in range(0,16,2)]==expected,name
source=(r/'build-vita/azel_upstream/battle_battleDamageDisplay.cpp').read_text()
producer=source[source.index('void createHitSparkEffect('):]
producer=producer[:producer.index('\n}')+2]
fixture=r'''#include <cassert>
#include <cstdint>
#include <cstdio>
using s16=int16_t; using s32=int32_t;
struct Vec {int x,y,z;};
struct s_workAreaCopy {}; using p_workArea=s_workAreaCopy*;
struct sBattleTargetable {s16 m58; int m5E_impactForce,m60; Vec* m4_pPosition;};
struct sDamageDisplayTask {
 struct TypedTaskDefinition {void* init;void(*update)(sDamageDisplayTask*);void(*draw)(sDamageDisplayTask*);void* destroy;};
 s16 m22_damageValue=0;int m42_colorType=0;Vec m10_position{};
};
static sDamageDisplayTask output;static bool fail;static int formatted,mode;
void damageDisplay_update(sDamageDisplayTask*){} void damageDisplay_draw(sDamageDisplayTask*){}
template<class T>T* createSiblingTaskWithCopy(s_workAreaCopy*,const typename T::TypedTaskDefinition*){output={};return fail?nullptr:&output;}
void damageDisplay_formatAndPosition(sDamageDisplayTask*,int m){++formatted;mode=m;}
'''
main=r'''
int main(){
 s_workAreaCopy owner;Vec position{12,-34,56};
 for(int value=-32767;value<=32767;++value)for(int impact=0;impact<4;++impact)for(int strength: {0,10,11,255}){
  sBattleTargetable target{static_cast<s16>(value),impact,strength,&position};formatted=0;
  createHitSparkEffect(&owner,target,1);
  const int color=value<0?1:value==0?3:((impact==1||impact==2)&&strength>10?2:0);
  assert(output.m22_damageValue==(value<0?-value:value));assert(output.m42_colorType==color);
  assert(formatted==1&&mode==1);assert(output.m10_position.x==12&&output.m10_position.y==-34&&output.m10_position.z==56);
 }
 fail=true;formatted=0;sBattleTargetable target{0,1,20,&position};createHitSparkEffect(&owner,target,0);assert(formatted==0);
 puts("PASS shared hit display: zero/heal/normal/critical values, live position and allocation failure");
}
'''
with tempfile.TemporaryDirectory(prefix='lagi-hit-display-') as tmp:
 d=Path(tmp);cpp=d/'test.cpp';cpp.write_text('#include <initializer_list>\n'+fixture+producer+main)
 subprocess.run(['cl','/nologo','/std:c++20','/EHsc',str(cpp),'/Fe:'+str(d/'test.exe'),'/Fo:'+str(d/'test.obj')],check=True)
 subprocess.run([str(d/'test.exe')],check=True)
print('PASS native A3 and Lathum zero-damage instruction contracts')

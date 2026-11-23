from pathlib import Path
import subprocess,tempfile
r=Path(__file__).resolve().parents[1]
f=(r/'tests/nanyd_motion_test.cpp').read_text().split('struct PositionBlock',1)[0]
f=f.replace('struct fixedPoint{','struct fixedPoint{friend fixedPoint operator+(fixedPoint a,fixedPoint b){return lagi::formationAdd(a.m_value,b.m_value);}')
f+=r"""
struct sAnimRingSubTask{sVec3_FP m8_center,m14_current,m20_target,m38_direction;int m44=0,m48_lifetime=12;};
int randomCalls=0;u32 randomNumber(){++randomCalls;return 0x2000;}
fixedPoint getCos(u32){return 0;}
fixedPoint MTH_Mul(fixedPoint a,fixedPoint b){return lagi::formationMultiply(a.asS32(),b.asS32());}
int performDivision(int divisor,int value){return value/divisor;}
std::vector<std::array<sVec3_FP,2>> lines;
void displayRaySegmentFromWorldSpace(sVec3_FP* p,int width,unsigned source,unsigned size,unsigned color,unsigned mode){assert(!width&&!source&&!size&&color==0xFFFF&&mode==8);lines.push_back({p[0],p[1]});}
// PRODUCTION
int main(){
 sAnimRingSubTask task;task.m8_center={100,200,300};task.m14_current={1100,2200,3300};task.m20_target={2100,4200,6300};
 randomCalls=0;animRingSubTask_Draw(&task);assert(lines.size()==10&&randomCalls==27);
 for(int i=0;i<10;++i)for(int axis=0;axis<3;++axis){const int delta=(task.m14_current[axis].asS32()-task.m8_center[axis].asS32())/10;assert(lines[i][0][axis].asS32()==task.m8_center[axis].asS32()+i*delta);assert(lines[i][1][axis].asS32()==task.m8_center[axis].asS32()+(i+1)*delta);}
 for(int lifetime:{3,12,30,93})for(int frame=0;frame<lifetime;++frame){task.m48_lifetime=lifetime;task.m44=frame;lines.clear();randomCalls=0;animRingSubTask_DrawAlt(&task);const int begin=frame*10/lifetime,end=begin+3,count=std::max(0,std::min(9,end)-begin);assert(lines.size()==size_t(count+(end!=9))&&randomCalls==count*3);for(size_t i=1;i<lines.size();++i)for(int axis=0;axis<3;++axis)assert(lines[i-1][1][axis].asS32()==lines[i][0][axis].asS32());}
 puts("PASS production shared line effects: ten-segment default, moving alternate window, continuity, native jitter RNG count and shared line service arguments");
}
""".replace('// PRODUCTION',(r/'src/integration/lagi_field_anim_ring_draw.inc').read_text())
with tempfile.TemporaryDirectory(prefix='lagi-ring-draw-') as d:
 p=Path(d);(p/'test.cpp').write_text(f)
 subprocess.run(['cl','/nologo','/EHsc','/std:c++20','/I'+str(r/'include'),str(p/'test.cpp'),'/Fe:'+str(p/'test.exe'),'/Fo:'+str(p/'test.obj')],check=True)
 subprocess.run([str(p/'test.exe')],check=True)

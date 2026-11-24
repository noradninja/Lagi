from pathlib import Path
import subprocess,tempfile
r=Path(__file__).resolve().parents[1];s=(r/'build-vita/azel_upstream/field_fieldDebrisScatter.cpp').read_text()
a=s.index('static void initBonesFromHierarchy(');b=s.index('// 0607e252',a);bones=s[a:b]
a=s.index('    // The descriptor owns the hierarchy/pose keys;');b=s.index('    // Build initial transform matrix',a);pose=s[a:b]
f=r"""
#include <vector>
#include <cassert>
#include <cstdint>
#include <cstdio>
using u16=uint16_t;using s32=int32_t;
struct sVec3_FP{int m0_X=0,m4_Y=0,m8_Z=0;};
struct sMatrix4x3{int m[3][4]{};};void copyMatrix(sMatrix4x3* a,sMatrix4x3* b){*b=*a;}
void translateMatrix(sVec3_FP p,sMatrix4x3* m){m->m[0][3]+=p.m0_X;m->m[1][3]+=p.m4_Y;m->m[2][3]+=p.m8_Z;}
void rotateMatrixZYX(sVec3_FP* p,sMatrix4x3*){assert(!p->m0_X&&!p->m4_Y&&!p->m8_Z);}
void extractRotationFromMatrix(sVec3_FP& p,sMatrix4x3&){p={};}
struct sProcessed3dModel{};struct sModelHierarchy{sProcessed3dModel* m0_3dModel; sModelHierarchy* m4_subNode=nullptr; sModelHierarchy* m8_nextNode=nullptr;};
struct Pose{sVec3_FP m0_translation,mC_rotation;};struct s_3dModel{std::vector<Pose> m2C_poseData;};
struct sDebrisBoneData{sProcessed3dModel* m0_model;sVec3_FP m4_position,m1C_rotation;};
struct sDebrisScatterTask{std::vector<sDebrisBoneData> m_bones;struct Task{bool finished=false;void markFinished(){finished=true;}}task;Task* getTask(){return &task;}};
bool fail=false;int steps=0;std::vector<Pose> poses{{{10,20,30},{}},{{1,2,3},{}},{{4,5,6},{}}};
struct Bundle{std::vector<Pose>* getStaticPose(int key,int count){assert(key==0xB50&&count==3);return &poses;}};
struct Params{int m34_modelOffset=0x18,m36_poseOffset=0xB50;};
bool init3DModelRawData(void*,s_3dModel* m,int flags,Bundle*,int key,void* animation,std::vector<Pose>* pose,void* color,void* hotpoints){assert(!flags&&key==0x18&&!animation&&!color&&!hotpoints);if(fail)return false;m->m2C_poseData=*pose;return true;}
void stepAnimation(s_3dModel*){++steps;}
// BONES
void* run(sDebrisScatterTask* pTask){void* parent=pTask;Bundle bundle;auto* pBundle=&bundle;Params descriptor;auto* params=&descriptor;int numBones=3;
// POSE
 sProcessed3dModel model;sModelHierarchy child{&model},sibling{&model},root{&model,&child,&sibling};sMatrix4x3 matrix;matrix.m[0][3]=100;matrix.m[1][3]=200;matrix.m[2][3]=300;u16 index=0;pTask->m_bones.resize(3);
 initBonesFromHierarchy(pTask,&root,index,matrix,&lagiPoseModel);assert(index==3);
 return pTask;
}
int main(){sDebrisScatterTask task;assert(run(&task)==&task&&steps==1&&!task.task.finished);assert(task.m_bones[0].m4_position.m0_X==110&&task.m_bones[0].m4_position.m4_Y==220);assert(task.m_bones[1].m4_position.m0_X==111&&task.m_bones[1].m4_position.m8_Z==333);assert(task.m_bones[2].m4_position.m0_X==104&&task.m_bones[2].m4_position.m4_Y==205);fail=true;sDebrisScatterTask bad;assert(!run(&bad)&&bad.task.finished&&steps==1);puts("PASS generated shared debris initialization: descriptor pose binding, posed child/sibling origins and failed-init retirement");}
""".replace('// BONES',bones).replace('// POSE',pose)
assert 'initBonesFromHierarchy(pTask, pHierarchy, boneIdx, rootMatrix, &lagiPoseModel);' in s
with tempfile.TemporaryDirectory(prefix='lagi-debris-pose-') as d:
 p=Path(d);(p/'test.cpp').write_text(f)
 subprocess.run(['cl','/nologo','/EHsc','/std:c++20',str(p/'test.cpp'),'/Fe:'+str(p/'test.exe'),'/Fo:'+str(p/'test.obj')],check=True)
 subprocess.run([str(p/'test.exe')],check=True)

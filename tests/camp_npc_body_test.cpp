#include <array>
#include <bit>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <vector>
using s32=std::int32_t;using u32=std::uint32_t;
struct fixedPoint {
    s32 value=0;fixedPoint(s32 v=0):value(v){} s32 asS32()const{return value;}
    operator s32()const{return value;}
};
struct sVec3_FP {
    std::array<fixedPoint,3> value{};
    fixedPoint& operator[](unsigned i){return value[i];}const fixedPoint& operator[](unsigned i)const{return value[i];}
    void zeroize(){value={};}
    sVec3_FP operator-(const sVec3_FP& b)const {sVec3_FP r;for(int i=0;i<3;++i)r[i]=s32(value[i])-s32(b[i]);return r;}
};
struct Pose {sVec3_FP m0_translation,mC_rotation;};
struct s_3dModel {std::vector<Pose> m2C_poseData,m48_poseDataInterpolation;};
struct sSaturnPtr {s32 address;};
struct Overlay {sSaturnPtr getSaturnPtr(s32 address){return {address};}} overlay;
Overlay* gCurrentTownOverlay=&overlay;
struct Bundle {
    struct Hierarchy {int countNumberOfBones(){return 8;}} hierarchy;
    struct StaticPose{} pose;
    Hierarchy* getModelHierarchy(int index){assert(index==8);return &hierarchy;}
    StaticPose* getStaticPose(int index,int count){assert(index==0x70&&count==8);return &pose;}
};
struct sNPC {Bundle* m0_fileBundle{};struct {sVec3_FP m0_position,mC_rotation;}mE8;
    std::array<fixedPoint,2>m20_lookAtAngle;s_3dModel m34_3dModel;};
template<class T,class A>struct s_workAreaTemplateWithArgWithCopy {
    Bundle* m0_fileBundle{};
    struct TypedTaskDefinition{void(*init)(T*,A);void(*update)(T*);void(*draw)(T*);void(*destroy)(T*);};
};
template<class T,class A>T* createSubTaskWithArgWithCopy(sNPC* parent,A arg){
    auto* t=new T;t->m0_fileBundle=parent->m0_fileBundle;T::getTypedTaskDefinition()->init(t,arg);return t;
}
void init3DModelRawData(void*,s_3dModel* model,int flags,Bundle*,int index,void*,Bundle::StaticPose*,void*,void*){
    assert(!flags&&index==8);model->m2C_poseData.resize(8);
    for(int i=0;i<8;++i)for(int a=0;a<3;++a)model->m2C_poseData[i].m0_translation[a]=100+i*10+a;
}
struct sMonsterBodyPart {sVec3_FP m10_translation,m34_rotationAcceleration;};
static int bodyDeletes=0,updates=0,draws=0,scopes=0;
struct sMonsterBody {
    sVec3_FP m0_translation,mC_rotation,m18_rotationTarget;
    void(*m24_update)(sMonsterBodyPart*,const sVec3_FP*,const sVec3_FP*,const sVec3_FP*);
    void(*m28_draw)(Bundle*,sMonsterBodyPart*);std::vector<sMonsterBodyPart>m30_parts;
    ~sMonsterBody(){++bodyDeletes;}
};
static sVec3_FP lastBase,lastTarget;
static void updateBody(sMonsterBodyPart*,const sVec3_FP*,const sVec3_FP* base,const sVec3_FP* target){
    ++updates;lastBase=*base;lastTarget=*target;
}
static void drawBody(Bundle*,sMonsterBodyPart*){assert(scopes==1);++draws;}
sMonsterBody* monsterBody_create(void*,int count){assert(count==7);auto*b=new sMonsterBody;b->m30_parts.resize(count);b->m24_update=updateBody;b->m28_draw=drawBody;return b;}
void monsterBody_loadPartData(sMonsterBody*,int mode,sSaturnPtr p){assert(mode==1&&p.address==0x607EAFC);}
void monsterBody_initPose(sMonsterBody*b,const sVec3_FP* position,const sVec3_FP* rotation){b->m0_translation=*position;b->mC_rotation=*rotation;b->m18_rotationTarget=*rotation;}
namespace lagi::azel_bridge {void begin_view_relative_submission_scope(){++scopes;}void end_view_relative_submission_scope(){--scopes;}}
struct Operation {char kind;s32 angle;};static std::vector<Operation>operations;
struct sMatrix4x3 {s32 m[3][4]{};sVec3_FP getTranslation()const{sVec3_FP r;for(int i=0;i<3;++i)r[i]=m[i][3];return r;}};
void initMatrixToIdentity(sMatrix4x3*m){*m={};operations.clear();}
void translateMatrix(const sVec3_FP& v,sMatrix4x3*m){operations.push_back({'t',v[0]});for(int i=0;i<3;++i)m->m[i][3]+=v[i];}
void rotateMatrixShiftedX(s32 a,sMatrix4x3*){operations.push_back({'x',a});}
void rotateMatrixShiftedY(s32 a,sMatrix4x3*){operations.push_back({'y',a});}
void rotateMatrixShiftedZ(s32 a,sMatrix4x3*){operations.push_back({'z',a});}
void rotateMatrixZYX(const sVec3_FP*,sMatrix4x3*){operations.push_back({'r',0});}
fixedPoint MTH_Mul(fixedPoint a,s32 b){return s32((std::int64_t(a.asS32())*b)>>16);}
sVec3_FP readSaturnVec3(sSaturnPtr p){assert(p.address==0x607EB88);sVec3_FP v;v[0]=7;v[1]=8;v[2]=9;return v;}
fixedPoint atan_FP(s32){return 10;}fixedPoint atan2_FP(s32,s32){static int i=0;return (++i&1)?20:30;}
void transformVec(const sVec3_FP& in,sVec3_FP& out,const sMatrix4x3&){out=in;}
#include "production.inc"
int main(){
    Bundle bundle;sNPC parent;parent.m0_fileBundle=&bundle;parent.m34_3dModel.m2C_poseData.resize(3);
    parent.mE8.m0_position[0]=1000;parent.m20_lookAtAngle[0]=0x1000;parent.m20_lookAtAngle[1]=0x10000;
    for(int i=0;i<3;++i)for(int a=0;a<3;++a)parent.m34_3dModel.m2C_poseData[i].m0_translation[a]=i+1+a;
    auto* task=lagiCreateCampNpcBody(&parent);assert(task->parent==&parent);
    assert(operations.size()==16&&operations[0].kind=='t');
    assert(operations[2].kind=='x'&&operations[3].angle==0x8000000);
    assert(operations[8].angle==0x4CCC&&operations[12].angle==0xB333);
    assert(operations[13].angle==0x1000&&operations[15].angle==s32(0xFE5843F2u));
    assert(task->position[0]==1013&&task->position[1]==17&&task->position[2]==21);
    for(int i=0;i<7;++i)assert(task->body->m30_parts[i].m10_translation[2]==102+i*10);
    const auto base=task->body->mC_rotation;
    parent.m34_3dModel.m48_poseDataInterpolation.resize(3);
    parent.m34_3dModel.m48_poseDataInterpolation[0].m0_translation[0]=70;
    parent.mE8.m0_position[2]=5;
    sLagiCampNpcBody::Update(task);
    assert(updates==1&&task->position[0]==1077&&task->previousPosition[0]==1013);
    assert(task->body->mC_rotation[1]==base[1]&&lastBase[2]==base[2]);
    const auto& acceleration=task->body->m30_parts[0].m34_rotationAcceleration;
    assert(acceleration[0]==(-7*16384)&&acceleration[1]==0&&acceleration[2]==64*16384);
    sLagiCampNpcBody::Draw(task);assert(draws==1&&!scopes);
    sLagiCampNpcBody::Delete(task);assert(bodyDeletes==1&&!task->body);delete task;
    puts("PASS production Camp child: native resource/seed calls, normal/interpolated attachment service order, movement impulses, base/target separation, draw scope and teardown; not matrix or hardware parity");
}

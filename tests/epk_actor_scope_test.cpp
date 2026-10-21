#include <cassert>
#include <cstdio>
static bool scoped;static int depth,draws,logs,translations;static unsigned submissions;
namespace lagi::azel_bridge {
static void begin_view_relative_submission_scope(){assert(!scoped && depth==0);scoped=true;}
static void end_view_relative_submission_scope(){assert(scoped && depth==0);scoped=false;}
static unsigned submission_count(){return submissions;}
}
namespace lagi::platform::logging {static void writef(const char*,...){assert(!scoped && depth==0);++logs;}}
static void pushCurrentMatrix(){assert(scoped);++depth;}
static void popMatrix(){assert(scoped && depth==1);--depth;}
struct Vec{void zeroize(){assert(scoped);}};
static void translateCurrentMatrix(Vec&){assert(scoped && depth==1);++translations;}
struct Pose{Vec m0_translation;};
struct Model{int m12_numBones=4;Pose m2C_poseData[1];void(*m18_drawFunction)(Model*);};
static void modelDraw(Model*){assert(scoped && depth==1);++draws;submissions+=4;}
struct sCutsceneCommandDefaultTask{
 Model m8;unsigned lagiDrawReports=0;
#include "epk_draw.inc"
};
struct sE011Task1{
 Model mC_3dModel;Vec m5C;
#include "attached_draw.inc"
};
int main(){
 sCutsceneCommandDefaultTask a,b;a.m8.m18_drawFunction=b.m8.m18_drawFunction=modelDraw;
 for(int i=0;i<4;++i){sCutsceneCommandDefaultTask::Draw(&a);sCutsceneCommandDefaultTask::Draw(&b);}
 assert(draws==8 && logs==4 && submissions==32 && !scoped && depth==0);
 sE011Task1 attached;attached.mC_3dModel.m18_drawFunction=modelDraw;sE011Task1::Draw(&attached);
 assert(draws==9 && translations==1 && !scoped && depth==0);
 puts("PASS generated generic EPK and E011 attached-actor draw scopes, matrix ordering and bounded per-actor diagnostics; not hardware pixels");
}

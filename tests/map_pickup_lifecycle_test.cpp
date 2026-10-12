#include <cassert>
#include <cstdio>
using s32=int; using s16=short; using s8=signed char;
struct Vec { int v[3]{}; int& operator[](int i){return v[i];} };
struct Bone { Vec m0_translation,mC_rotation; };
struct sStaticPoseData { Bone m0_bones[2]; };
struct Bundle { sStaticPoseData pose; sStaticPoseData* getStaticPose(int,int){return &pose;} };
struct sLCSTarget { int m18_diableFlags=0; };
struct s_itemBoxType1 {
 struct {Bundle* m0_mainMemoryBundle;} m0;
 sLCSTarget m8_LCSTarget;
 int m20=77,m21=88,mEA_state=0,m90=0,m94=-0x444444;
 int m78_scale=1,m7C_invScale=1,m8B_LCSType=1,m88_poseIdx=0xB2,m8D_visibilityFlag=0;
 Vec m3C_pos,m6C_rotation,m60_renderPosition;
};
using p_workArea=s_itemBoxType1*;
struct { bool owned=false; bool getBit(int bit){assert(bit==0x1A5); return owned;} } mainGameState;
Vec LCSItemBox_Table6[3];
bool spinning=true; int registrations=0;
bool LCSItemBox_shouldSpin(s_itemBoxType1*){return spinning;}
void updateLCSTarget(sLCSTarget* t){if(!(t->m18_diableFlags&1)) ++registrations;}
void pushCurrentMatrix(){} void popMatrix(){}
void translateCurrentMatrix(Vec*){} void translateCurrentMatrix(Vec){}
void rotateCurrentMatrixZYX(Vec*){} void rotateCurrentMatrixZYX(Vec){}
void scaleCurrentMatrixRow0(int){} void scaleCurrentMatrixRow1(int){} void scaleCurrentMatrixRow2(int){}
void rotateCurrentMatrixShiftedX(int){} void rotateCurrentMatrixShiftedY(int){} void rotateCurrentMatrixShiftedZ(int){}
void transformAndAddVecByCurrentMatrix(Vec*,Vec*){}
void gridCellDraw_normalSub2(Bundle*,int,int){}
#include "map_pickup_production.inc"
int main(){
 Bundle bundle; s_itemBoxType1 box; box.m0.m0_mainMemoryBundle=&bundle;
 LCSItemBox_UpdateType1(&box); assert(registrations==1 && box.mEA_state==0);
 LCSItemBox_Callback1(&box,&box.m8_LCSTarget); assert(box.mEA_state==1);
 // The shared laser reward is delayed. Native rotation finishes independently.
 for(int i=0;i<100 && box.mEA_state==1;++i) LCSItemBox_UpdateType1(&box);
 assert(box.mEA_state==2 && box.m90==0);
 // Mock inventory ownership delivery, not the real laser task or persistence.
 mainGameState.owned=true; int before=registrations;
 for(int i=0;i<100;++i) LCSItemBox_UpdateType1(&box);
 assert(registrations==before && (box.m8_LCSTarget.m18_diableFlags&1));
 assert(box.m20==77 && box.m21==88);
 mainGameState.owned=false; box.m8_LCSTarget.m18_diableFlags=0;
 box.m8D_visibilityFlag=1; LCSItemBox_UpdateType1(&box);
 assert(registrations==before && (box.m8_LCSTarget.m18_diableFlags&1));
 puts("PASS: actual map callback/update rotation, ownership disable, native visibility; laser reward and device rendering untested");
}

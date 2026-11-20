#include <array>
#include <bit>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <map>
#include <vector>
using s8=std::int8_t;using u8=std::uint8_t;using s16=std::int16_t;using u16=std::uint16_t;using s32=std::int32_t;using u32=std::uint32_t;
struct fixedPoint {s32 value=0;fixedPoint(s32 v=0):value(v){}s32 asS32()const{return value;}operator s32()const{return value;}};
using sVec3_FP=std::array<fixedPoint,3>;
struct sSaturnPtr {s32 m_offset=0;sSaturnPtr operator+(s32 n)const{return {m_offset+n};}};
static std::map<s32,s32>memory;
u8 readSaturnU8(sSaturnPtr p){return u8(memory[p.m_offset]);}
u16 readSaturnU16(sSaturnPtr p){return u16(memory[p.m_offset]);}
s32 readSaturnS32(sSaturnPtr p){return memory[p.m_offset];}
sSaturnPtr readSaturnEA(sSaturnPtr p){return {memory[p.m_offset]};}
sVec3_FP readSaturnVec3(sSaturnPtr p){return {memory[p.m_offset],memory[p.m_offset+4],memory[p.m_offset+8]};}
struct Bundle {
    struct Hierarchy {int countNumberOfBones(){return 17;}}hierarchy;
    struct Pose{}pose;int collision=0;
    Hierarchy* getModelHierarchy(unsigned n){assert(n==4);return &hierarchy;}
    Pose* getStaticPose(unsigned n,int count){assert(n==0x4C&&count==17);return &pose;}
    void* getCollisionModel(unsigned n){assert(n==0x20);++collision;return this;}
    void* get3DModel(unsigned n){assert(n==0x54);return this;}
};
struct s_3dModel {std::vector<int>m48_poseDataInterpolation;};
using s_fileBundle=Bundle;using sStaticPoseData=Bundle::Pose;
struct sNPC;
using p_workArea=sNPC*;
struct sCollisionBody {
    sVec3_FP* m30_pPosition{};sVec3_FP* m34_pRotation{};sNPC* m38_pOwner{};
    sSaturnPtr m3C_scriptEA;void* m40{};int setup=0;
};
struct sNPC {
    Bundle* m0_fileBundle{};s8 mC=7,mD=7,mE_controlState=7,mF=7;
    sSaturnPtr m10_InitPtr,m18,m30_animationTable;void(*m14_updateFunction)(sNPC*){};
    s32 m1C{};std::array<fixedPoint,2>m20_lookAtAngle;fixedPoint m28_animationLeftOver,m2C_currentAnimation;
    struct {sVec3_FP m0_position,mC_rotation;}mE8;
    sCollisionBody m84;s_3dModel m34_3dModel;fixedPoint m148_savedAngle;
    s16 m14C_inputFlags=7,m14E=7;s32 m150_inputX=7,m154_inputY=7;
    u8 m158_animQueue[32]{};s8 m178=7,m179=7,m17A=7,m17B=7;
};
struct Slot {sNPC* workArea{};sNPC* pNPC{};};struct {std::array<Slot,16>m70_npcPointerArray;}npcData0;
struct FileOwner {Bundle* m0_fileBundle{};};struct FileEntry{FileOwner* mC_fileBundle{};};static std::array<FileEntry,1>dramAllocatorEnd;
static void lagiCampNpcDecay(sNPC*){}static void lagiCampNpcPosePlayback(sNPC*){}
static void lagiCampNpcPatrol(sNPC*){}static void lagiCampNpcWeightedAnimation(sNPC*){}void updateEdgePosition(sNPC*){}
void setCollisionSetup(sCollisionBody* body,int setup){body->setup=setup;}
void setCollisionBounds(sCollisionBody*,sVec3_FP lo,sVec3_FP hi){assert(lo[0]==-20&&hi[0]==20);}
static bool loaded=false;static int inits=0,children=0,shadows=0,normalDraws=0,interpolatedDraws=0,stack=0,scopes=0;
bool isDataLoaded(s32 file){assert(file==0x28);return loaded;}
void init3DModelRawData(sNPC*,s_3dModel*,int flags,Bundle*,unsigned model,void* animation,Bundle::Pose*,void*,void*){assert(flags==0x100&&model==4&&!animation);++inits;}
void lagiCreateCampNpcBody(sNPC*){++children;}
namespace lagi::azel_bridge {void begin_view_relative_submission_scope(){++scopes;}void end_view_relative_submission_scope(){--scopes;}}
void pushCurrentMatrix(){++stack;}void popMatrix(){--stack;}
void translateCurrentMatrix(sVec3_FP){}void rotateCurrentMatrixShiftedX(fixedPoint){}void rotateCurrentMatrixShiftedY(fixedPoint){}
void addObjectToDrawList(void*){assert(scopes==1&&stack==1);++shadows;}
void applyEdgeAnimation(s_3dModel*,std::array<fixedPoint,2>*){assert(scopes==1&&stack==1);++normalDraws;}
void applyEdgeAnimation2(s_3dModel*,std::array<fixedPoint,2>*){assert(scopes==1&&stack==1);++interpolatedDraws;}
#include "production_init.inc"
struct sLagiCampNPC:sNPC {
    void(*m_UpdateMethod)(sLagiCampNPC*){};void(*m_DrawMethod)(sLagiCampNPC*){};
    static void Update2(sLagiCampNPC*){}
#include "production_methods.inc"
};
int main(){
    Bundle bundle,common;FileOwner owner{&common};dramAllocatorEnd[0].mC_fileBundle=&owner;
    const sSaturnPtr arg{0x607B8AC};memory[arg.m_offset]=0x28;
    memory[arg.m_offset+0x20]=3;memory[arg.m_offset+0x22]=4;memory[arg.m_offset+0x24]=0x4C;
    memory[arg.m_offset+0x28]=0x10000;memory[arg.m_offset+0x2C]=0x607FA84;
    memory[arg.m_offset+8]=11;memory[arg.m_offset+12]=22;memory[arg.m_offset+16]=33;
    memory[arg.m_offset+0x34]=3;memory[arg.m_offset+0x36]=0x20;
    memory[arg.m_offset+0x38]=0x605DFc3;memory[arg.m_offset+0x3C]=-20;memory[arg.m_offset+0x48]=20;
    memory[0x607FA86]=0x54;
    using Actor=sLagiCampNPC;
    for(int mode=0;mode<5;++mode)for(int flags : {0,0x40,0x80,0xC0}) {
        Actor npc;npc.m0_fileBundle=&bundle;loaded=false;
        memory[arg.m_offset+0x21]=mode|flags;
        memory[arg.m_offset+0x30]=mode==3?0x6070EB8:0x607FE40;
        Actor::Init(&npc,arg);
        assert(npcData0.m70_npcPointerArray[3].workArea==&npc&&npcData0.m70_npcPointerArray[3].pNPC==&npc);
        assert(npc.mC==0&&npc.mD==mode&&u8(npc.mF)==(flags&0x80));
        assert(npc.mE_controlState==(mode==1?1:0)&&npc.m14E==(mode==4?0x100:0));
        assert(npc.m14C_inputFlags==0&&npc.m150_inputX==0&&npc.m178==0&&npc.m17A==0);
        assert(npc.m84.m38_pOwner==&npc&&npc.m84.m30_pPosition==&npc.mE8.m0_position&&npc.m84.m40==&bundle);
        assert(npc.mE8.m0_position[1]==22&&npc.m84.m3C_scriptEA.m_offset==0x605DFc3);
        const auto oldInit=inits,oldChildren=children;
        Actor::Update(&npc);assert(inits==oldInit&&!npc.m_DrawMethod);
        loaded=true;Actor::Update(&npc);
        assert(inits==oldInit+1&&children==oldChildren+!!(flags&0x40));
        assert(npc.m_UpdateMethod==&Actor::Update2&&npc.m_DrawMethod==&Actor::Draw2);
        const auto oldShadow=shadows;Actor::Draw2(&npc);
        assert(shadows==oldShadow+!(flags&0x80)&&!scopes&&!stack);
        npc.m34_3dModel.m48_poseDataInterpolation.push_back(1);Actor::Draw2(&npc);
        assert(!scopes&&!stack);
        Actor::Delete(&npc);assert(!npcData0.m70_npcPointerArray[3].workArea&&!npcData0.m70_npcPointerArray[3].pNPC);
        Actor replacement;npcData0.m70_npcPointerArray[3]={&replacement,&replacement};
        Actor::Delete(&npc);assert(npcData0.m70_npcPointerArray[3].workArea==&replacement&&npcData0.m70_npcPointerArray[3].pNPC==&replacement);
    }
    assert(inits==20&&normalDraws==20&&interpolatedDraws==20&&children==10);
    puts("PASS production Camp NPC init/resource-wait/draw/delete: all5 control modes, shadow/child flags, pose banks, typed registry/collision links and replacement-owner protection; active animation/patrol covered separately; hardware pending");
}

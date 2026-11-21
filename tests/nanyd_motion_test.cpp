#include <array>
#include <vector>
#include <cstdint>
#include <cstring>
#include <cassert>
#include <fstream>
#include <iterator>
#include <cstdio>
#include "lagi/native_formation_motion.h"
using s16=std::int16_t;using u8=std::uint8_t;using u32=std::uint32_t;using s32=std::int32_t;
struct fixedPoint{s32 m_value{};fixedPoint(s32 n=0):m_value(n){}s32 asS32()const{return m_value;}s32 getInteger()const{return m_value>>16;}
 fixedPoint& operator+=(fixedPoint b){m_value=lagi::formationAdd(m_value,b.m_value);return *this;}};
struct sVec3_FP:std::array<fixedPoint,3>{friend sVec3_FP operator-(const sVec3_FP& a,const sVec3_FP& b){sVec3_FP v;for(int i=0;i<3;++i)v[i]=lagi::formationSubtract(a[i].asS32(),b[i].asS32());return v;}sVec3_FP& operator+=(const sVec3_FP& b){for(int i=0;i<3;++i)(*this)[i]+=b[i];return *this;}};
static_assert(sizeof(sVec3_FP)==12);
struct sSaturnPtr{u32 address{};sSaturnPtr operator+(u32 n)const{return {address+n};}};
std::vector<u8> prg;
u8 readSaturnU8(sSaturnPtr p){return prg.at(p.address-0x06054000);}
s32 word(sSaturnPtr p){u32 v=0;for(unsigned i=0;i<4;++i)v=(v<<8)|readSaturnU8(p+i);return lagi::formationSignedBits(v);}
sVec3_FP readSaturnVec3(sSaturnPtr p){sVec3_FP v;for(unsigned i=0;i<3;++i)v[i]=word(p+i*4);return v;}
struct PositionBlock{sVec3_FP m0_position;u8 mC_pad[12]{};};
struct Group{sSaturnPtr m8_dataTable;unsigned mF_deadCount{};};
struct sBTL_A5_FormationTask{PositionBlock m2C_posBlock;u8 mF0_flag{};u8 mF4_entityPositions[216]{};sSaturnPtr m1CC_dataTable2;Group mE0_entityGroup;};
struct s_battleEngine{sVec3_FP mC_battleCenter;};
unsigned parentUpdates{};
void formationUpdatePosition(PositionBlock* p,sVec3_FP* center){p->m0_position=*center;++parentUpdates;}
struct FakeTask{bool finished{};void markFinished(){finished=true;}};
struct FakeMeter{FakeTask task;unsigned m31{};FakeTask* getTask(){return &task;}};
struct sBattleTargetable{sVec3_FP* m8{};};
struct Model{};
using sVec2_FP=std::array<fixedPoint,2>;
struct sBTL_A5_Enemy{
 FakeTask task;FakeTask* getTask(){return &task;}
 FakeMeter* mAC_lifeMeter{};
 sBattleTargetable* m_lagiRushTarget{};
 sVec2_FP m_lagiRushHeading{};
 bool m_lagiCoordinatedHit{};
 u8 m28_state{};u32 m14_flags{};s16 m24_counter{},m17C_hitFlashCount=7,m17E_hitFlashColor=2;
 std::array<Model,1> m2C_models;

 sBTL_A5_FormationTask* m8_formation{};unsigned m1C_entityIndex{};
 sVec3_FP m60_targetPosition,m170_scatterOffset,m198_orbitVelocity,m90_initialPosition,m30_position;
 sVec3_FP m_lagiPositionVelocity,m_lagiPositionAcceleration,m_lagiAngularVelocity,m_lagiAngularAcceleration;
 sVec3_FP m84_displayRotation,m9C_targetRotation,m3C_rotation;
 s32 m4C_velocityY{},m6C_deathRotX{},m70_deathRotY{};
};
// PRODUCTION_TRAIL
// PRODUCTION_MOTION
static sVec3_FP headAt(int frame){sVec3_FP v;if(frame<0)return v;
 v[0]=0x10000+frame*0x100;v[1]=-0x3000+(frame%11)*0x200;v[2]=-0x20000-frame*0x180;return v;}
static void same(const sVec3_FP& a,const sVec3_FP& b){for(unsigned axis=0;axis<3;++axis)assert(a[axis].asS32()==b[axis].asS32());}
int main(int argc,char** argv){assert(argc==2);std::ifstream stream(argv[1],std::ios::binary);assert(stream);prg.assign(std::istreambuf_iterator<char>(stream),{});
 unsigned cases=0;
 for(unsigned count:{20u,10u}){
  sBTL_A5_FormationTask formation;formation.mE0_entityGroup.m8_dataTable={count==20?0x060B0D24u:0x060B0F18u};
  formation.m1CC_dataTable2={0x060B0F04};
  std::vector<sBTL_A5_Enemy> actual(count),expected(count);
  for(unsigned i=0;i<count;++i){auto& a=actual[i];a.m8_formation=&formation;a.m1C_entityIndex=i;
   a.m90_initialPosition=readSaturnVec3(formation.mE0_entityGroup.m8_dataTable+i*12);
   a.m84_displayRotation[1]=0x08000000;a.m9C_targetRotation=a.m84_displayRotation;
   a.m198_orbitVelocity[0]=s32(i)*17-23;a.m198_orbitVelocity[1]=s32(i)*-7;a.m198_orbitVelocity[2]=s32(i)*19;
   if(i%3==0){a.m170_scatterOffset[0]=0x1000;a.m170_scatterOffset[1]=-0x14000;a.m170_scatterOffset[2]=0x2000;}
   expected[i]=a;
  }
  for(int frame=0;frame<400;++frame){
   s_battleEngine engine;engine.mC_battleCenter=headAt(frame);
   sVec3_FP rotation;rotation[0]=frame*3;rotation[1]=-frame*5;rotation[2]=frame*7;
   std::memcpy(formation.m2C_posBlock.mC_pad,&rotation,12);
   const auto before=parentUpdates;lagiUpdateNanydFormationTrail(&formation,&engine);assert(parentUpdates==before+1);
   // Independent time-index oracle distinguishes head-before-copy from a FIFO.
   for(unsigned slot=0;slot<18;++slot){sVec3_FP history;
    std::memcpy(&history,formation.mF4_entityPositions+slot*12,12);
    same(history,headAt(frame-int(slot>0?slot-1:0)));
   }
   for(unsigned i=0;i<count;++i){auto& e=expected[i];const unsigned map=readSaturnU8(formation.m1CC_dataTable2+i);assert(map<18);
    const auto center=headAt(frame-int(map>0?map-1:0));
    auto target=readSaturnVec3(formation.mE0_entityGroup.m8_dataTable+i*12);target+=e.m170_scatterOffset;target+=e.m198_orbitVelocity;
    for(unsigned axis=0;axis<3;++axis){
     lagi::NativeFormationSpringAxis p{target[axis].asS32(),e.m90_initialPosition[axis].asS32(),e.m_lagiPositionVelocity[axis].asS32(),0,0};
     lagi::stepNativeFormationSpring(p,center[axis].asS32(),0xFF,0xFFF);
     e.m90_initialPosition[axis]=p.current;e.m_lagiPositionVelocity[axis]=p.velocity;e.m30_position[axis]=p.world;
     lagi::NativeFormationSpringAxis a{e.m84_displayRotation[axis].asS32(),e.m9C_targetRotation[axis].asS32(),e.m_lagiAngularVelocity[axis].asS32(),0,0};
     lagi::stepNativeFormationAngularSpring(a,rotation[axis].asS32(),0xFF,0xFFF);
     e.m9C_targetRotation[axis]=a.current;e.m_lagiAngularVelocity[axis]=a.velocity;e.m3C_rotation[axis]=a.world;
    }
    lagiUpdateNanydPosition(&actual[i]);const auto& a=actual[i];
    same(a.m60_targetPosition,target);same(a.m30_position,e.m30_position);same(a.m90_initialPosition,e.m90_initialPosition);
    same(a.m_lagiPositionVelocity,e.m_lagiPositionVelocity);same(a.m3C_rotation,e.m3C_rotation);same(a.m9C_targetRotation,e.m9C_targetRotation);
    assert(a.m4C_velocityY==e.m_lagiPositionVelocity[1].asS32());
    assert(a.m6C_deathRotX==e.m_lagiAngularVelocity[0].asS32()&&a.m70_deathRotY==e.m_lagiAngularVelocity[1].asS32());
    for(unsigned axis=0;axis<3;++axis)assert(a.m_lagiPositionAcceleration[axis].asS32()==0&&a.m_lagiAngularAcceleration[axis].asS32()==0);
    ++cases;
   }
  }
 }
 std::printf("PASS Nanyd native table/map binding, head-before-copy history, shared position/angular springs and live death-field synchronization: %u actor frames\n",cases);
}

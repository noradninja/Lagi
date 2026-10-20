#include <cassert>
#include <cstdint>
#include <cstdio>
struct Vec {
 int v[3]{};int& operator[](int n){return v[n];}const int& operator[](int n)const{return v[n];}
 void zeroize(){for(auto& n:v)n=0;}
 Vec& operator+=(const Vec& r){for(int i=0;i<3;++i)v[i]+=r[i];return *this;}
 Vec& operator-=(const Vec& r){for(int i=0;i<3;++i)v[i]-=r[i];return *this;}
};
Vec operator-(Vec a,const Vec& b){return a-=b;}
int MTH_Mul(int a,int b){return int((std::int64_t(a)*b)>>16);}
Vec MTH_Mul(Vec a,int b){for(int i=0;i<3;++i)a[i]=MTH_Mul(a[i],b);return a;}
int getSin(int angle){assert(!(angle%1024));return angle==1024?65536:angle==3072?-65536:0;}
int getCos(int angle){return getSin((angle+1024)&4095);}
struct Dragon {Vec m8_position{{100000,-200000,300000}};} dragon;
struct Overlay {Dragon* m18_dragon=&dragon;} overlay;
struct Manager {Overlay* m10_battleOverlay=&overlay;} manager,*gBattleManager=&manager;
struct sArachnothFormation {Vec m2F0_eatDragonVelocity,m2FC_eatDragonAcceleration,m308_eatDragonPosition,m230,m2CC,m224_translation;int m2C4_currentAttackDelay=0,m2C0_currentAttackState=1,m278_targetedQuadrant=0;};
Vec *current=nullptr,*desired=nullptr;int resets=0;
void battleEngine_setCurrentCameraPositionPointer(Vec* v){current=v;}
void battleEngine_setDesiredCameraPositionPointer(Vec* v){desired=v;}
void battleEngine_resetCameraInterpolation(){++resets;}
#include "../src/integration/lagi_arachnoth_devour.inc"
int main(){
 sArachnothFormation a;
 for(int frame=1;frame<=25;++frame){a.m2C4_currentAttackDelay=frame;
  Vec expectedAccel=a.m2FC_eatDragonAcceleration;
  expectedAccel+=MTH_Mul(dragon.m8_position-a.m308_eatDragonPosition,0x28F);
  expectedAccel-=MTH_Mul(expectedAccel,0x1999);
  Vec expectedPos=a.m308_eatDragonPosition;expectedPos+=expectedAccel;
  lagiArachnothDevourApproach(&a);
  for(int i=0;i<3;++i){assert(a.m2FC_eatDragonAcceleration[i]==expectedAccel[i]);assert(a.m308_eatDragonPosition[i]==expectedPos[i]);assert(a.m2F0_eatDragonVelocity[i]==0);}
  assert(a.m2C0_currentAttackState==(frame==25?2:1));
 }
 for(int q=0;q<4;++q){a.m278_targetedQuadrant=q;lagiArachnothDevourCloseup(&a);
  assert(current==&a.m2CC&&desired==&a.m224_translation);
  assert(a.m2CC[0]==dragon.m8_position[0]+MTH_Mul(0x4000,getSin(q*1024)));
  assert(a.m2CC[1]==dragon.m8_position[1]+0xA000);
  assert(a.m2CC[2]==dragon.m8_position[2]+MTH_Mul(0x4000,getCos(q*1024)));}
 assert(resets==4);puts("PASS production Devour approach, all axes, native damping, frame-25 phase change and four-quadrant close-up; no device acceptance");
}

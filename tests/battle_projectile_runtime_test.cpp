#include <array>
#include <vector>
#include <map>
#include <cstdint>
#include <cassert>
#include <cstdio>
using u8=uint8_t;using u16=uint16_t;using u32=uint32_t;
using s8=int8_t;using s16=int16_t;using s32=int32_t;
struct fixedPoint{
    s32 m_value{};fixedPoint(s32 x=0):m_value(x){}
    static fixedPoint fromInteger(s32 x){return x*65536;}
    s32 getInteger()const{return m_value>>16;}
    fixedPoint& operator+=(fixedPoint x){m_value+=x.m_value;return *this;}
    friend fixedPoint operator+(fixedPoint a,fixedPoint b){return a.m_value+b.m_value;}
    friend fixedPoint operator-(fixedPoint a,fixedPoint b){return a.m_value-b.m_value;}
    friend fixedPoint operator-(fixedPoint a){return -a.m_value;}
    friend bool operator<(fixedPoint a,fixedPoint b){return a.m_value<b.m_value;}
};
struct sVec3_FP:std::array<fixedPoint,3>{
    sVec3_FP& operator+=(const sVec3_FP& b){for(int i=0;i<3;++i)(*this)[i]+=b[i];return *this;}
    friend sVec3_FP operator-(const sVec3_FP& a,const sVec3_FP& b){sVec3_FP d;for(int i=0;i<3;++i)d[i]=a[i]-b[i];return d;}
};
using sVec2_FP=std::array<fixedPoint,2>;
fixedPoint MTH_Mul(fixedPoint a,fixedPoint b){return s32((int64_t(a.m_value)*b.m_value)>>16);}
fixedPoint FP_Div(fixedPoint a,fixedPoint b){return s32(int64_t(a.m_value)*65536/b.m_value);}
fixedPoint dot3_FP(const sVec3_FP* a,const sVec3_FP* b){fixedPoint v;for(int i=0;i<3;++i)v+=MTH_Mul((*a)[i],(*b)[i]);return v;}
fixedPoint getCos(int){return 65536;}fixedPoint getSin(int){return 0;}
void computeLookAt(const sVec3_FP&,sVec2_FP& a){a={};}
struct sSaturnPtr{u32 address{};sSaturnPtr operator+(u32 n)const{return {address+n};}bool isNull()const{return !address;}};
struct File{sSaturnPtr getSaturnPtr(u32 a){return {a};}}file;
auto* g_BTL_A3=&file;
u8 readSaturnU8(sSaturnPtr p){return u8(p.address&255);}
u16 readSaturnU16(sSaturnPtr p){return u16(p.address);}
s16 readSaturnS16(sSaturnPtr p){return s16(p.address);}
fixedPoint readSaturnFP(sSaturnPtr p){return s32(p.address);}
sSaturnPtr readSaturnEA(sSaturnPtr p){return p;}
unsigned draws=0,modelSteps=0,quadSteps=0,modelDraws=0,quadDraws=0,deletedModels=0,damages=0;
u32 randomNumber(){++draws;return 2;}
struct sVdp1Quad{u32 identity;};
struct sAnimatedQuad{const std::vector<sVdp1Quad>* frames{};};
std::vector<sVdp1Quad> initVdp1Quad(sSaturnPtr p){return {{p.address}};}
void particleInitSub(sAnimatedQuad* a,u16 slot,const std::vector<sVdp1Quad>* q){assert(slot==0x1234);a->frames=q;}
int sGunShotTask_UpdateSub4(sAnimatedQuad*){++quadSteps;return 0;}
int drawProjectedParticle(sAnimatedQuad*,sVec3_FP*){++quadDraws;return 0;}
struct Hierarchy{unsigned countNumberOfBones(){return 3;}}hierarchy;
struct Pose{}pose;struct Anim{}animation;
struct Bundle{
    Hierarchy* getModelHierarchy(u16){return &hierarchy;}
    Pose* getStaticPose(u16,unsigned n){assert(n==3);return &pose;}
    Anim* getAnimation(u16){return &animation;}
}bundle;
struct Allocation{u16 m4_vdp1Memory=0x1234;}allocation;
struct Control{bool finished=false;void markFinished(){finished=true;}bool isFinished()const{return finished;}};
struct s_workArea{virtual ~s_workArea()=default;Control control;Control* getTask(){return &control;}};using p_workArea=s_workArea*;
struct Owner:s_workArea{Bundle* m0_fileBundle=&bundle;Allocation* m4_vd1Allocation=&allocation;}owner;
template<class T>struct s_workAreaTemplateWithCopy:Owner{
    struct TypedTaskDefinition{void* init;void(*update)(T*);void(*draw)(T*);void(*remove)(T*);};
    void(*m_UpdateMethod)(T*){};void(*m_DrawMethod)(T*){};
};
struct Entry{Owner* mC_fileBundle=&owner;};Entry dramAllocatorEnd[16];
struct Dragon:Owner{sVec3_FP m8_position{};int m8C{};}dragon;
struct Engine{sVec3_FP m1A0_battleAutoScrollDelta{};}engine;
struct Overlay{Dragon* m18_dragon=&dragon;Engine* m4_battleEngine=&engine;}overlay;
struct Manager{Overlay* m10_battleOverlay=&overlay;}manager;
auto* gBattleManager=&manager;
template<class T>T* createSubTaskWithCopy(Dragon* p,const typename T::TypedTaskDefinition* d){assert(p==&dragon);auto* t=new T;t->m_UpdateMethod=d->update;t->m_DrawMethod=d->draw;return t;}
struct s_3dModel{void(*m18_drawFunction)(s_3dModel*){};~s_3dModel(){++deletedModels;}};
bool modelOK=true;
bool init3DModelRawData(p_workArea,s_3dModel* m,int,Bundle*,u16,Anim*,Pose*,void*,void*){m->m18_drawFunction=[](s_3dModel*){++modelDraws;};return modelOK;}
void stepAnimation(s_3dModel*){++modelSteps;}
void pushCurrentMatrix(){}void popMatrix(){}
void translateCurrentMatrix(sVec3_FP*){}void rotateCurrentMatrixYXZ(sVec3_FP*){}
void transformAndAddVecByCurrentMatrix(sVec3_FP* a,sVec3_FP* b){*b=*a;}
void applyDamageToDragon(int&,s32 damage,sVec3_FP&,s32 type,const sVec3_FP&,s32 damageType){assert(damage==9&&type==3&&damageType==2);++damages;}
struct Emission{const std::vector<sVdp1Quad>* quad;sVec3_FP position;};std::vector<Emission> emissions;
void createParticleEffect(Owner*,const std::vector<sVdp1Quad>* q,const sVec3_FP* p,void*,void*,int,void*,int){emissions.push_back({q,*p});}
std::vector<int> sounds;
void playSystemSoundEffect(s8 sound){sounds.push_back(sound);}
#include "lagi_battle_projectile_descriptor.inc"
#include "lagi_battle_projectile_motion.inc"
#include "projectile_runtime.inc"
using s_workAreaCopy=Owner;
struct sUrchin:Owner{sVec3_FP m8{};};
template<class T>using s_workAreaTemplate=s_workAreaTemplateWithCopy<T>;
template<class T>T* createSubTask(p_workArea,const typename T::TypedTaskDefinition* d){auto* t=new T;t->m_UpdateMethod=d->update;return t;}
#include "lagi_battle_urchin_projectiles.inc"
int main(){
    sLagiBattleSphere a{},b{};a.position[0]=1000000;b.position[0]=1100000;
    b.radius=5000;assert(!lagiBattleSpheresOverlap(a,b));
    b.position[0]=1005000;assert(lagiBattleSpheresOverlap(a,b));
    b.position[0]=1005010;assert(!lagiBattleSpheresOverlap(a,b));
    sLagiBattleProjectileDescriptor d;d.travel=4;d.verticalAcceleration=-3;
    sVec3_FP source{},target{},scroll{},velocity{},acceleration{};
    target[0]=40;target[1]=80;target[2]=120;scroll[0]=1;
    lagiBattleProjectileMotion(source,target,d,scroll,velocity,acceleration);
    assert(velocity[0].m_value==11&&velocity[1].m_value==28&&velocity[2].m_value==30);
    d.slot=12;d.hierarchy=12;d.pose=0xDC;d.lifetime=1;d.damage=9;d.damageType=2;
    d.hit={0x300};d.trail={0x400};d.trailMask=1;d.hitSlot=12;d.trailSlot=12;
    dragon.m8_position[0]=2000000;target=source;
    auto* task=static_cast<sLagiBattleProjectile*>(lagiCreateBattleProjectile(source,target,d));
    assert(task->model&&draws==0);lagiBattleProjectileDraw(task);assert(modelDraws==1);
    lagiBattleProjectileUpdate(task);assert(!task->control.finished&&task->timer==0&&modelSteps==1&&draws==1&&emissions.size()==1);
    lagiBattleProjectileUpdate(task);assert(task->control.finished&&emissions.size()==3&&damages==0);
    auto* saved=emissions.back().quad;lagiBattleProjectileDelete(task);delete task;
    assert(deletedModels==1&&!saved->empty()); // hit particles outlive projectile
    emissions.clear();d.hierarchy=0;d.quad={0x200};d.trail={};d.lifetime=10;
    source={};dragon.m8_position={};target=source;
    task=static_cast<sLagiBattleProjectile*>(lagiCreateBattleProjectile(source,target,d));
    assert(!task->model&&quadSteps==2);lagiBattleProjectileDraw(task);assert(quadDraws==1);
    lagiBattleProjectileUpdate(task);
    assert(damages==1&&task->control.finished&&emissions.size()==2); // native double hit/expiry
    lagiBattleProjectileDelete(task);delete task;
    modelOK=false;d.hierarchy=12;
    task=static_cast<sLagiBattleProjectile*>(lagiCreateBattleProjectile(source,target,d));
    assert(task->control.finished&&!task->m_UpdateMethod&&!task->m_DrawMethod);
    lagiBattleProjectileDelete(task);delete task;assert(deletedModels==2);
    modelOK=true;sUrchin enemy;enemy.m8[0]=500000;
    for(bool alternate:{false,true})for(bool strong:{false,true}){
        auto* monitor=static_cast<sLagiUrchinProjectileMonitor*>(lagiCreateUrchinProjectileEffect(&enemy,alternate,strong));
        assert(monitor->projectiles->size()==size_t(alternate?(strong?16:8):(strong?20:8)));
        assert(sounds.back()==0x68&&lagiUrchinEffectPosition(&enemy)==&enemy.m8);
        lagiUrchinProjectileMonitorUpdate(monitor);assert(!monitor->control.finished);
        // Cancelling the enemy-owned monitor never finishes dragon-owned shots.
        auto shots=*monitor->projectiles;
        lagiUrchinProjectileMonitorDelete(monitor);delete monitor;
        for(auto* p:shots){
            auto* shot=static_cast<sLagiBattleProjectile*>(p);
            assert(!shot->control.finished&&shot->position[0].m_value==500000);
            lagiBattleProjectileDelete(shot);delete shot;
        }
    }
    auto* monitor=static_cast<sLagiUrchinProjectileMonitor*>(lagiCreateUrchinProjectileEffect(&enemy,false,false));
    for(auto* p:*monitor->projectiles)p->getTask()->markFinished();
    auto shots=*monitor->projectiles;
    lagiUrchinProjectileMonitorUpdate(monitor);assert(monitor->control.finished);
    lagiUrchinProjectileMonitorDelete(monitor);delete monitor;
    for(auto* p:shots){auto* shot=static_cast<sLagiBattleProjectile*>(p);lagiBattleProjectileDelete(shot);delete shot;}
    std::puts("Native projectile radius, motion, visuals, expiry, impact and model cleanup PASS");
}

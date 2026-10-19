#include <array>
#include <vector>
#include <cassert>
#include <cstdint>
#include <cmath>
#include <cstdio>
using s32=int32_t;using u32=uint32_t;using s16=int16_t;using u16=uint16_t;using s8=int8_t;using u8=uint8_t;
struct fixedPoint { s32 v;fixedPoint(s32 n=0):v(n){}s32 asS32()const{return v;}operator s32()const{return v;}fixedPoint& operator-=(s32 n){v-=n;return *this;} };
static fixedPoint MTH_Mul(fixedPoint a,fixedPoint b){return fixedPoint(int64_t(a.v)*b.v>>16);}
static fixedPoint FP_Div(s32 a,fixedPoint b){return fixedPoint(int64_t(a)*65536/b.v);}
static fixedPoint intDivide(s32 a,s32 b){return fixedPoint(b/a);}
static fixedPoint getSin(s32 a){return fixedPoint(std::lround(std::sin(a*6.283185307179586/4096)*65536));}
struct sVec3_FP { std::array<fixedPoint,3> v; sVec3_FP(s32 x=0,s32 y=0,s32 z=0):v{x,y,z}{}fixedPoint& operator[](int i){return v[i];}const fixedPoint& operator[](int i)const{return v[i];}
 sVec3_FP operator-(const sVec3_FP& b)const{return {v[0]-b[0],v[1]-b[1],v[2]-b[2]};}sVec3_FP& operator+=(const sVec3_FP& b){for(int i=0;i<3;++i)v[i]=v[i]+b[i];return *this;}};
using quadColor=std::array<u16,4>;
struct sSaturnPtr {u32 m_offset{};sSaturnPtr operator+(s32 n)const{return {m_offset+u32(n)};}};
static std::array<u8,0x10000> memory;
static size_t off(sSaturnPtr p){return p.m_offset-0x06054000;}
static u8 readSaturnU8(sSaturnPtr p){return memory.at(off(p));}static s8 readSaturnS8(sSaturnPtr p){return readSaturnU8(p);}
static u16 readSaturnU16(sSaturnPtr p){return (readSaturnU8(p)<<8)|readSaturnU8(p+1);}static s16 readSaturnS16(sSaturnPtr p){return readSaturnU16(p);}
static s32 readSaturnS32(sSaturnPtr p){return (u32(readSaturnU16(p))<<16)|readSaturnU16(p+2);}static fixedPoint readSaturnFP(sSaturnPtr p){return readSaturnS32(p);}
static sSaturnPtr readSaturnEA(sSaturnPtr p){return {u32(readSaturnS32(p))};}
static sVec3_FP readSaturnVec3(sSaturnPtr p){return {readSaturnS32(p),readSaturnS32(p+4),readSaturnS32(p+8)};}
static void put16(u32 a,u16 n){memory.at(a-0x06054000)=n>>8;memory.at(a-0x06054000+1)=n;}
static void put32(u32 a,u32 n){put16(a,n>>16);put16(a+2,n);}
struct Control{bool finished{};void markFinished(){finished=true;}};
struct Allocation{u16 m4_vdp1Memory=0x100;}allocation;
struct Owner{Allocation* m4_vd1Allocation=&allocation;};using p_workArea=Owner*;
static Owner owner;static Owner* currentResTask=&owner;
struct Entry{Owner* mC_fileBundle=&owner;};static std::array<Entry,4> dramAllocatorEnd;
template<class T>struct s_workAreaTemplate:Owner{Control control;Control* getTask(){return &control;}struct TypedTaskDefinition{void* init;void(*update)(T*);void(*draw)(T*);void(*remove)(T*);};};
template<class T>using s_workAreaTemplateWithCopy=s_workAreaTemplate<T>;
template<class T>static T* createSubTask(Owner*,const typename T::TypedTaskDefinition*){return new T;}
template<class T>static T* createSubTaskWithCopy(Owner* p,const typename T::TypedTaskDefinition* d){return createSubTask<T>(p,d);}
static struct {sSaturnPtr getSaturnPtr(u32 a){return {a};}} overlay;
static auto* gTWN_E011=&overlay;
static bool stopped;static int remaining=100,clockReads,sounds;
static int e006_scriptFunction_605861eSub0(){return stopped;}
static int lagiE011Remaining(){++clockReads;return remaining;}
static int lagiE011EffectTicks(s16& previous){if(stopped)return 1;int n=previous-lagiE011Remaining();previous=lagiE011Remaining();return n;}
static void playSystemSoundEffect(int n){assert(n==7);++sounds;}
static u32 randomNumber(){return 0;}
static s32 performModulo(s32 divisor,s32 value){return value%divisor;}
struct sMatrix4x3{};static void initMatrixToIdentity(sMatrix4x3*){}static void rotateMatrixYXZ(sVec3_FP*,sMatrix4x3*){}
static void computeLookAt(const sVec3_FP&,sVec3_FP& rotation){rotation={};}
static void transformAndAddVec(const sVec3_FP& a,sVec3_FP& b,const sMatrix4x3&){b=a;}
static std::vector<u32> calls;
static p_workArea lagiE011CreateScheduledQuad(sSaturnPtr p){calls.push_back(p.m_offset);return nullptr;}
static p_workArea lagiE011CreateScheduledModel(sSaturnPtr p){calls.push_back(p.m_offset);return nullptr;}
static int draws;
static void lagiDisplayWorldStripCheck(std::array<sVec3_FP,2>&,std::array<fixedPoint,2>&,u16 address,s16 size,u16 palette,const quadColor* c,s32 mode){assert(address==0x120 && size==0x208 && palette==0x140 && mode==8);assert((*c)[0]==(*c)[1] && (*c)[2]==(*c)[3]);++draws;}
void lagiDisplayWorldStrip(std::array<sVec3_FP,2>& p,std::array<fixedPoint,2>& w,u16 a,s16 s,u16 c,const quadColor* q,s32 m){lagiDisplayWorldStripCheck(p,w,a,s,c,q,m);}
#include "lagi_e011_ribbon_scheduler.inc"
struct sVdpVar1 {u8* m8_destination{};s16 mC_size{};u8 mF_isPending{};sVdpVar1* m10_nextTransfert{};};
sVdpVar1* vdpVar2=nullptr; sVdpVar1* vdpVar3=nullptr;
static std::array<u8,0x80000> vram;static std::array<u8,0x1000> cram;
static u8* getVdp2Vram(u32 offset){return vram.data()+offset;}static u8* getVdp2Cram(u32 offset){return cram.data()+offset;}
static u8* getSaturnPtr(sSaturnPtr p){return memory.data()+off(p);}
static int unpacks,tableInits,paramInits,copies;static u32 copiedEA,copiedBytes,cramOffset;
static void unpackGraphicsToVDP2(u8* source,u8* destination){assert(source==getSaturnPtr({0x060607D8}) || source==getSaturnPtr({0x06060926}));assert(destination==getVdp2Vram(0x1f000));++unpacks;}
static void asyncDmaCopy(sSaturnPtr p,void* target,u32 bytes,int mode){assert(mode==0);copiedEA=p.m_offset;copiedBytes=bytes;cramOffset=static_cast<u8*>(target)-cram.data();++copies;}
static struct Regs{u16 m20_BGON{};}regs;
static struct {Regs* m4_pendingVdp2Regs=&regs;int m_isDirty{};}vdp2Controls;
struct sE011BackgroundTask {
 s32 m60_state{},m74_cutscenePos{},m70_counter{},m58_wavePhase{},m5C_scrollAccum{},m44_dltOmega{},m48_dltAng{},m4C_ratio{},m50_lineSpd{},m54_upSpd{},m68_offsetX{},m6C_offsetY{};
 sVec3_FP lagiTrackingPoint; sVec3_FP* m64_positionPtr{};
 static void initCinematicBarTable(sE011BackgroundTask*){++tableInits;}
 static void initCinematicBarParams(sE011BackgroundTask*){++paramInits;}
}background;
static auto* gE011BackgroundTask=&background;
struct Fade{int m0_color;};static struct{Fade m0_fade0{11},m24_fade1{22};}g_fadeControls;
struct FadeCall{Fade* owner;int start;u16 target;int frames;};static std::vector<FadeCall> fades;
static int convertColorToU32ForFade(int color){return color+100;}
static void fadePalette(Fade* owner,int start,u16 target,int frames){fades.push_back({owner,start,target,frames});}
#include "lagi_e011_later_callbacks.inc"
int main(){
 constexpr u32 descriptor=0x06055000,table=0x06056000;
 put16(descriptor,1);put32(descriptor+0x10,0x20000);put32(descriptor+0x20,0x20000);put16(descriptor+0x42,7);
 put16(descriptor+0x24,0x20);put16(descriptor+0x26,0x208);put16(descriptor+0x28,0x40);
 for(int c=0;c<3;++c){put16(descriptor+0x36+c*2,4);put16(descriptor+0x3c+c*2,24);}
 auto* ribbon=static_cast<sE011Ribbon*>(lagiE011CreateRibbon({descriptor}));
 assert(ribbon->colors[0]==u16(0x8000|4|4<<5|4<<10));assert(ribbon->colors[19]==u16(0x8000|24|24<<5|24<<10));
 lagiE011RibbonDraw(ribbon);assert(draws==19);
 lagiE011RibbonUpdate(ribbon);assert(sounds==1 && ribbon->remaining==0x10000 && ribbon->history[0][0]==0);
 lagiE011RibbonUpdate(ribbon);assert(ribbon->remaining==0x10000); // delta zero
 --remaining;lagiE011RibbonUpdate(ribbon);assert(ribbon->history[0][0]==0x10000);
 --remaining;lagiE011RibbonUpdate(ribbon);assert(ribbon->state==2 && ribbon->tail==30 && ribbon->history[0][0]==0x20000);
 for(int i=0;i<30;++i){--remaining;lagiE011RibbonUpdate(ribbon);}assert(ribbon->state==3 && !ribbon->control.finished);
 --remaining;lagiE011RibbonUpdate(ribbon);assert(ribbon->control.finished && sounds==1);delete ribbon;
 // Multiple due records dispatch in one frame, then stop at the future threshold.
 put32(table,100);put32(table+4,1);put32(table+8,0x060589E2);
 put32(table+12,100);put32(table+16,2);put32(table+20,0x06058B2A);
 put32(table+24,90);put32(table+28,3);put32(table+32,0x060589E2);put32(table+36,0);
 sE011Schedule schedule;schedule.table={table};remaining=100;sE011Schedule::Update(&schedule);assert((calls==std::vector<u32>{1,2}) && !schedule.control.finished);
 remaining=90;sE011Schedule::Update(&schedule);assert((calls==std::vector<u32>{1,2,3}) && !schedule.control.finished);
 sE011Schedule::Update(&schedule);assert(schedule.control.finished);
 sE011Schedule stop;stop.table={table};stopped=true;sE011Schedule::Update(&stop);assert(stop.control.finished && calls.size()==3);
 stopped=false;constexpr u32 palette=0x06057000,frames=0x06057100,wave=0x06057200,track=0x06057300;
 put16(palette,2);put32(palette+4,frames);memory[palette+8-0x06054000]=3;
 put32(frames,0x06057400);put16(frames+4,2);put32(frames+8,0);
 sE011PaletteSequence sequence;sequence.descriptor={palette};sequence.previous=101;remaining=100;
 sE011PaletteSequence::Update(&sequence);assert(copies==1 && copiedEA==0x06057400 && copiedBytes==0x200 && cramOffset==0x400);
 remaining=99;sE011PaletteSequence::Update(&sequence);assert(copies==1 && !sequence.control.finished);
 remaining=98;sE011PaletteSequence::Update(&sequence);assert(sequence.control.finished && copies==1);
 sVdpVar1 head,transfer,tail;head.m10_nextTransfert=&transfer;transfer.m8_destination=getVdp2Vram(0x1f000);transfer.mC_size=4;transfer.m10_nextTransfert=&tail;tail.m8_destination=getVdp2Vram(0x24000);vdpVar2=&head;vdpVar3=&tail;
 for(int i=0;i<6;++i)put32(wave+i*4,i+11);
 lagiE011SetScreenWave(wave);assert(unpacks==1 && tableInits==1 && background.m60_state==1 && background.m70_counter==16 && background.m44_dltOmega==11 && background.m54_upSpd==15);
 assert(head.m10_nextTransfert==&tail && tail.m10_nextTransfert==nullptr);
 lagiE011SetScreenWave(wave);assert(unpacks==1 && tableInits==2); // same mode keeps decompressed screen
 put32(track,123);put32(track+4,456);put32(track+8,789);put16(track+12,u16(-12));put16(track+14,34);put32(track+16,55);
 lagiE011SetScreenTracking(track);assert(unpacks==2 && paramInits==1 && background.m60_state==2 && background.m70_counter==55 && background.m68_offsetX==-12 && background.m6C_offsetY==34);
 assert(background.m64_positionPtr==&background.lagiTrackingPoint && (*background.m64_positionPtr)[2]==789);
 stopped=true;lagiE011SetScreenWave(wave);lagiE011SetScreenTracking(track);assert(unpacks==2 && tableInits==2 && paramInits==1);
 for(int mask=0;mask<4;++mask){fades.clear();lagiE011FadeColor(0x1a94a,40,mask);
  assert(fades.size()==unsigned((mask&1)+((mask>>1)&1)));
  for(auto& f:fades){assert(f.target==0xa94a && f.frames==40);assert(f.start==(f.owner==&g_fadeControls.m0_fade0?111:122));}}
 puts("PASS production E011 ribbon lifecycle, colors, draw resources, zero-delta clock, scheduler catch-up, sentinel and stop");
}

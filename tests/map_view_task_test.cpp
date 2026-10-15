#include "lagi/azel_map_menu.h"
#include "lagi/azel_books_menu.h"
#include <cassert>
#include <cstdio>
#include <vector>
using s32=std::int32_t;using u32=std::uint32_t;using s16=std::int16_t;
using u16=std::uint16_t;using u8=std::uint8_t;
struct fixedPoint {s32 raw;fixedPoint(s32 v):raw(v){}s32 asS32()const{return raw;}};
fixedPoint intDivide(s32 divisor,s32 value){return value/divisor;}
fixedPoint getSin(unsigned a){assert((a&0x3FF)==0);return a==0x400?65536:(a==0xC00?-65536:0);}
fixedPoint getCos(unsigned a){return getSin((a+0x400)&0xFFF);}
struct FakeTask {bool finished=false;void markFinished(){finished=true;}};
struct s_workArea {virtual ~s_workArea()=default;FakeTask task;FakeTask* getTask(){return &task;}};
template<class T,class Arg>struct s_workAreaTemplateWithArg:s_workArea {
    struct TypedTaskDefinition {void(*init)(T*,Arg);void(*update)(T*);void(*draw)(T*);void(*remove)(T*);};
};
template<class T,class Arg>T* createSubTaskWithArg(s_workArea*,Arg arg){auto* task=new T;T::Init(task,arg);return task;}
struct s_vdp1Command {
    u16 m0_CMDCTRL=0,m4_CMDPMOD=0,m6_CMDCOLR=0,m8_CMDSRCA=0,mA_CMDSIZE=0;
    s16 mC_CMDXA=0,mE_CMDYA=0,m10_CMDXB=0,m12_CMDYB=0,m14_CMDXC=0,m16_CMDYC=0,m18_CMDXD=0,m1A_CMDYD=0;
};
std::vector<s_vdp1Command> mainContextVdp1[2]={std::vector<s_vdp1Command>(1024),{}};
struct Packet {unsigned m4_bucketTypes=0;s_vdp1Command* m6_vdp1EA=nullptr;} packets[1024];
struct Context {std::vector<s_vdp1Command>::iterator m0_currentVdp1WriteEA;Packet* m20_pCurrentVdp1Packet=nullptr;unsigned m1C=0,mC=0;};
struct Buttons {u16 m8_newButtonDown=0,mC_newButtonDown2=0,m6_buttonDown=0;};
struct {
    Context m14_vdp1Context[2];
    struct Layer {s16 scrollX=0,scrollY=0;} m40BC_layersConfig[4];
    struct {struct {Buttons m0_current;} m0_inputDevices[1];} m4514;
    struct {u8 m6=0,m7=0;} m40AC;
} graphicEngineStatus;
struct Registers {u16 m10A_CCRNB=0xA580;} regs;
struct {Registers* m4_pendingVdp2Regs=&regs;int m_isDirty=0;} vdp2Controls;
struct Common {u8 definitions[40]{};u8* getSaturnPtr(unsigned address){assert(address==0x20E8BC);return definitions;}} common;
Common* gCommonFile=&common;
u16 readSaturnU16(const u8* p){return u16((u16(p[0])<<8)|p[1]);}
s16 readSaturnS16(const u8* p){return s16(readSaturnU16(p));}
void put(unsigned offset,unsigned value){common.definitions[offset]=value>>8;common.definitions[offset+1]=value&255;}
std::vector<int> sounds;
void playSystemSoundEffect(int sound){sounds.push_back(sound);}
bool presentationReady=true;unsigned setupCalls=0,markerCalls=0,restores=0;
std::array<s16,2> markerScroll{};
bool lagiAzelPresentMapPatterns(const u8* first,std::size_t a,const u8* second,std::size_t b,int slot,unsigned count){
    assert(a==0x800 && b==0x800 && first[0]==0x4A && second[0]==0x5B && slot==0 && count==1);
    ++setupCalls;return presentationReady;
}
std::array<s16,2> lagiAzelSetMapViewCenter(s16 x,s16 y){
    const auto& back=graphicEngineStatus.m40BC_layersConfig[1];
    const auto result=lagiAzelMapViewScrollOrigin(x,y,back.scrollX,back.scrollY);
    auto& map=graphicEngineStatus.m40BC_layersConfig[2];map.scrollX=result[0];map.scrollY=result[1];return result;
}
void lagiAzelSetupMapViewWindow(s16 x,s16 y){assert(x==0 && y==0);regs.m10A_CCRNB=(regs.m10A_CCRNB&0xFFE0)|31;}
unsigned lagiAzelDrawMapObjectMarkers(int slot,unsigned count,s16 x,s16 y,s16 offset){assert(slot==0 && count==1 && offset==0);++markerCalls;markerScroll={x,y};return 0;}
void lagiAzelRestoreMapViewPriorities(){++restores;}
#include "map_view_index_production.inc"
#include "map_view_task_production.inc"
static unsigned draw(LagiFieldMapViewTask& task)
{
    auto& context=graphicEngineStatus.m14_vdp1Context[0];
    context.m0_currentVdp1WriteEA=mainContextVdp1[0].begin();context.m20_pCurrentVdp1Packet=packets;context.m1C=context.mC=0;
    LagiFieldMapViewTask::Draw(&task);
    assert(context.m1C==context.mC);
    return context.mC;
}
int main()
{
    const unsigned definitions[4][5]={{0x20D0,0x108,0x5C,0x6C,0},{0x20D0,0x108,0x11C,0x6C,0x10},
        {0x20D4,0x108,0xBC,0xC,0},{0x20D4,0x108,0xBC,0xCC,0x20}};
    for(unsigned i=0;i<4;++i)for(unsigned word=0;word<5;++word)put(i*10+word*2,definitions[i][word]);
    LagiAzelMapViewSource source;
    source.resourcesReady=true;source.status=std::make_shared<LagiAzelMapViewStatus>();
    source.subfieldCount=1;source.context.subfield=0;source.context.x=256;source.context.y=256;
    source.context.descriptor.presentation={0,0,20,20};source.patterns[0]=0x4A;source.patterns[0x800]=0x5B;
    s_workArea parent;
    assert(!lagiAzelCreateFieldMapView(nullptr,source));
    auto* raw=lagiAzelCreateFieldMapView(&parent,source);assert(raw);
    auto& task=*static_cast<LagiFieldMapViewTask*>(raw);
    assert(task.centerX==8 && task.centerY==8);
    source.patterns[0]=0; // The task owns its copy, not this caller buffer.
    presentationReady=false;assert(draw(task)==0 && task.phase==0 && setupCalls==1);
    presentationReady=true;assert(draw(task)==0 && task.phase==1 && setupCalls==2);
    assert(source.status->setupSucceeded && !source.status->finished && (regs.m10A_CCRNB&31)==31);
    for(int remaining=15;remaining>0;--remaining){assert(draw(task)==0);assert(task.count==remaining && (regs.m10A_CCRNB&31)==remaining*2);}
    assert(draw(task)==5 && task.phase==2 && task.playerVisible);
    assert(regs.m10A_CCRNB==0xA580);
    auto arrow=mainContextVdp1[0][4];
    assert(arrow.m0_CMDCTRL==0x1002 && arrow.m4_CMDPMOD==0x80 && arrow.m6_CMDCOLR==0x700);
    assert(arrow.m8_CMDSRCA==0x23D0 && arrow.mA_CMDSIZE==0x220);
    assert(arrow.mC_CMDXA==-8 && arrow.mE_CMDYA==-40 && arrow.m10_CMDXB==-24 && arrow.m12_CMDYB==-40);
    assert(mainContextVdp1[0][1].m0_CMDCTRL==0x1010 && mainContextVdp1[0][3].m0_CMDCTRL==0x1020);
    task.source.context.heading=0x4000000;
    assert(draw(task)==5);
    arrow=mainContextVdp1[0][4];
    assert(arrow.mC_CMDXA==-40 && arrow.mE_CMDYA==-24 && arrow.m10_CMDXB==-40 && arrow.m12_CMDYB==-8);
    assert(arrow.m14_CMDXC==-9 && arrow.m16_CMDYC==-8 && arrow.m18_CMDXD==-9 && arrow.m1A_CMDYD==-24);
    task.source.context.heading=0;
    auto& input=graphicEngineStatus.m4514.m0_inputDevices[0].m0_current;
    input.mC_newButtonDown2=0x80;input.m6_buttonDown=0x80;
    assert(draw(task)==5 && task.phase==5 && task.centerX==9 && sounds.back()==10);
    input={};
    for(int step=1;step<=7;++step) {
        assert(draw(task)==5);
        assert(markerScroll[0]==96+step*4 && markerScroll[1]==160);
    }
    assert(task.phase==2 && task.savedScroll[0]==128 && task.previousX==9);
    assert(mainContextVdp1[0][4].mC_CMDXA==-40); // Arrow uses final snap, not step7's -36.
    // Both axes pan together and decrement precedence remains the shared native rule.
    input.mC_newButtonDown2=0x50;input.m6_buttonDown=0x50;
    draw(task);assert(task.centerX==8 && task.centerY==7 && task.phase==5);
    input={};for(int step=1;step<=7;++step)draw(task);
    assert(task.phase==2 && task.savedScroll==lagiAzelMapViewScrollOrigin(8,7,0,0));
    task.centerX=2;task.previousX=2;task.savedScroll=lagiAzelSetMapViewCenter(2,7);
    input.mC_newButtonDown2=0x40;input.m6_buttonDown=0x40;
    sounds.clear();draw(task);draw(task);assert((sounds==std::vector<int>{5}) && task.phase==2);
    input={};draw(task);input.mC_newButtonDown2=0x40;draw(task);assert(sounds.size()==2);
    // Cancel wins over movement; marker remains on this frame, then hides.
    input.m8_newButtonDown=1;input.mC_newButtonDown2=0x80;
    const auto before=task.centerX;draw(task);assert(task.phase==3 && task.centerX==before && sounds.back()==1);
    input={};
    for(int remaining=15;remaining>=0;--remaining){assert(draw(task)==0);assert((regs.m10A_CCRNB&31)==31-remaining*2);}
    assert(task.getTask()->finished && !source.status->finished);
    LagiFieldMapViewTask::Delete(&task);assert(source.status->finished && restores==1);delete raw;
    source.resourcesReady=false;assert(!lagiAzelCreateFieldMapView(&parent,source));
    source.resourcesReady=true;source.patterns[0]=0x4A;source.status=std::make_shared<LagiAzelMapViewStatus>();
    source.context.descriptor.presentation={10,0,0,20};
    raw=lagiAzelCreateFieldMapView(&parent,source);assert(raw && raw->getTask()->finished);
    LagiFieldMapViewTask::Delete(static_cast<LagiFieldMapViewTask*>(raw));delete raw;
    assert(source.status->finished && !source.status->setupSucceeded);
    std::puts("PASS: production Map view task reveal/pan/cancel/close, native arrow packets, owned buffers, shared input feedback and completion lifetime (host mocks; not Vita rendering)");
}

#include "lagi/azel_map_menu.h"
#include <cassert>
#include <cstdio>
struct {struct {unsigned m0_menuId=1;}m40AC;}graphicEngineStatus;
struct Game {int scene=0;bool story=false;int readPackedBits(int a,int b){assert(a==0 && b==2);return scene;}bool getBit(int a){assert(a==193);return story;}}mainGameState;
struct Field {int m2C_currentFieldIndex=2,m2E_currentSubFieldIndex=0;}field;
Field* currentField=&field;
Field* getFieldTaskPtr(){return currentField;}
struct Common {std::uint32_t m_base=0x200000;std::uint8_t* m_data;std::size_t m_dataSize;};
Common* gCommonFile=nullptr;
struct s_workArea{};using p_workArea=s_workArea*;
s_workArea*(*overlayDispatchTable[11])(s_workArea*,std::int32_t)={};
static s_workArea* worldBackend(s_workArea* p,std::int32_t){return p;}
bool contextAvailable=true,townOk=true;
int contextCalls=0,townCalls=0;
bool lagiAzelResolveCurrentMapContext(LagiAzelMapContext& context){++contextCalls;context={};if(contextAvailable){context.field=2;context.descriptor.nativeAddress=0x20E900;}return contextAvailable;}
bool lagiAzelResolveTownMapAvailability(LagiAzelMapAvailability& a){++townCalls;a={{0,1},1,false};return townOk;}
#include "source.inc"
s_workArea nativeTask,placeholder;
struct LagiFieldMapMenuTask{};
bool factoryOk=true;
int nativeCalls=0,fallbackCalls=0;
LagiAzelMapParentSource captured;
s_workArea* lagiAzelCreateMapParent(s_workArea*,const LagiAzelMapParentSource& source){++nativeCalls;captured=source;return factoryOk?&nativeTask:nullptr;}
template<class T>s_workArea* createSubTask(s_workArea*){++fallbackCalls;return &placeholder;}
namespace lagi::platform::logging {void writef(const char*,...) {}}
#include "dispatch.inc"
int main(){
 s_workArea parent;
 LagiAzelMapParentSource source;
 assert(lagiAzelResolveMapParentSource(source));
 assert(source.hasContext && source.availability.slots[0]==1 && source.availability.slots[1]==-1);
 assert(!source.standalone && source.availability.worldCover);
 assert(createMapTask(&parent)==&nativeTask && fallbackCalls==0);
 graphicEngineStatus.m40AC.m0_menuId=7;
 assert(lagiAzelResolveMapParentSource(source) && source.standalone);
 graphicEngineStatus.m40AC.m0_menuId=1;
 mainGameState.story=true;
 assert(!lagiAzelResolveMapParentSource(source) && !source.hasContext);
 assert(createMapTask(&parent)==&placeholder);
 std::vector<std::uint8_t> bytes(134788);
 Common common{0x200000,bytes.data(),bytes.size()};gCommonFile=&common;
 auto put=[&](unsigned p,std::uint32_t value){for(unsigned i=0;i<4;++i)bytes[p+i]=value>>(24-i*8);};
 // Null table and zero authored route have identical output for either gate.
 assert(lagiAzelResolveMapParentSource(source) && source.availability.slots[1]==0);
 put(0xA4D4+2*4,0x20A3E4);
 assert(lagiAzelResolveMapParentSource(source) && source.availability.slots[1]==0);
 put(0xA3E4,1);
 const int before=nativeCalls;
 assert(!lagiAzelResolveMapParentSource(source));
 assert(createMapTask(&parent)==&placeholder && nativeCalls==before);
 put(0xA3E4,0xFFFFFFFF);assert(!lagiAzelResolveMapParentSource(source));
 put(0xA3E4,0);assert(createMapTask(&parent)==&nativeTask);
 field.m2C_currentFieldIndex=1;
 assert(lagiAzelResolveMapParentSource(source));
 put(0xA3E4,9);assert(!lagiAzelResolveMapParentSource(source));
 mainGameState.story=false;assert(lagiAzelResolveMapParentSource(source));
 mainGameState.story=true;currentField=nullptr;assert(!lagiAzelResolveMapParentSource(source));
 currentField=&field;common.m_base=0x300000;assert(!lagiAzelResolveMapParentSource(source));
 common.m_base=0x200000;common.m_dataSize=4;assert(!lagiAzelResolveMapParentSource(source));
 common.m_dataSize=bytes.size();field.m2C_currentFieldIndex=-1;assert(!lagiAzelResolveMapParentSource(source));
 field.m2C_currentFieldIndex=2;field.m2E_currentSubFieldIndex=-1;assert(!lagiAzelResolveMapParentSource(source));
 field.m2E_currentSubFieldIndex=0;
 mainGameState.scene=2;contextAvailable=false;
 const int calls=contextCalls;
 assert(lagiAzelResolveMapParentSource(source) && !source.hasContext && source.availability.selected==1);
 assert(contextCalls==calls && townCalls>0);
 assert(!lagiAzelMapWorldDispatchAvailable() && createMapTask(&parent)==&placeholder);
 overlayDispatchTable[5]=worldBackend;
 assert(lagiAzelMapWorldDispatchAvailable() && createMapTask(&parent)==&nativeTask && captured.availability.slots[1]==1);
 townOk=false;assert(createMapTask(&parent)==&placeholder);townOk=true;
 for(int scene:{1,3}){
  mainGameState.scene=scene;
  assert(lagiAzelResolveMapParentSource(source) && source.availability.slots[0]==0 && source.availability.slots[1]==0);
 }
 factoryOk=false;assert(createMapTask(&parent)==&placeholder);
 std::puts("PASS: production Map live source and dispatch, proven gate-independent branches, both field tables, unresolved nonzero route rejection, native context/town/cadence and malformed state; no hardware validation");
}

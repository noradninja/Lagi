#include <cassert>
#include <cstdint>
#include <cstdio>
#include <string>
#include <typeinfo>
constexpr int SCE_O_WRONLY=1,SCE_O_CREAT=2,SCE_O_APPEND=4;
static bool failOpen;
static int opened,closed,aborts,throws;
static std::string output;
static void* thrownObject;
static std::type_info* thrownType;
static void(*thrownDestroy)(void*);
static int sceIoOpen(const char* path,int flags,int mode) {
    assert(std::string(path)=="ux0:data/lagi/lagi-fatal.log");
    assert(flags==7 && mode==0666);++opened;return failOpen?-1:42;
}
static int sceIoWrite(int fd,const void* data,int n) { assert(fd==42);output.append(static_cast<const char*>(data),n);return n; }
static int sceIoClose(int fd) { assert(fd==42);++closed;return 0; }
static unsigned sceKernelGetThreadId() { return 0x40010003; }
enum _Unwind_Reason_Code { _URC_NO_REASON,_URC_END_OF_STACK };
struct _Unwind_Context { uintptr_t ip; };
static uintptr_t _Unwind_GetIP(_Unwind_Context* c) { return c->ip; }
static _Unwind_Reason_Code _Unwind_Backtrace(_Unwind_Reason_Code(*callback)(_Unwind_Context*,void*),void* data) {
    for(int n=0;n<40;++n){_Unwind_Context c{uintptr_t(0x81000000+n*4)};if(callback(&c,data)==_URC_END_OF_STACK)return _URC_END_OF_STACK;}
    assert(false);return _URC_END_OF_STACK;
}
static void* testCaller() { return reinterpret_cast<void*>(uintptr_t(0x81234567)); }
#include "fatal_trace.inc"
struct Stop {};
extern "C" [[noreturn]] void __real_abort() { ++aborts;throw Stop{}; }
extern "C" [[noreturn]] void __real___cxa_throw(void* object,std::type_info* type,void(*destroy)(void*)) {
    ++throws;thrownObject=object;thrownType=type;thrownDestroy=destroy;throw Stop{};
}
static void destroy(void*) {}
int main() {
    lagiSetE011FatalStage("startScriptTask");
    try { __wrap_abort(); }catch(Stop&){}
    assert(aborts==1 && opened==1 && closed==1);
    assert(output.find("kind=abort")!=std::string::npos && output.find("lastE011Stage=startScriptTask")!=std::string::npos);
    assert(output.find("imageAbort=")!=std::string::npos && output.find("frame=31")!=std::string::npos && output.find("frame=32")==std::string::npos);
    output.clear();int object=123;auto* type=const_cast<std::type_info*>(&typeid(object));
    try { __wrap___cxa_throw(&object,type,destroy); }catch(Stop&){}
    assert(throws==1 && thrownObject==&object && thrownType==type && thrownDestroy==destroy);
    assert(output.find("kind=cxx-throw")!=std::string::npos);
    output.clear();failOpen=true;
    try { __wrap_abort(); }catch(Stop&){}
    assert(aborts==2 && output.empty() && opened==3 && closed==2);
    puts("PASS fatal-only synchronous trace, 32-frame bound, original abort/throw forwarding, file failure and stage context");
}

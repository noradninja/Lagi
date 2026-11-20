// Fatal-only diagnostics bypass the asynchronous log queue. A partial Vita
// core can end before every memory segment, including the main-thread stack.
#include <psp2/io/fcntl.h>
#include <psp2/kernel/threadmgr.h>
#if __has_include(<unwind.h>)
#include <unwind.h>
#define LAGI_HAS_UNWIND 1
#else
#define LAGI_HAS_UNWIND 0
#endif
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <typeinfo>
extern "C" [[noreturn]] void __wrap_abort();

namespace {
std::atomic_flag reporting=ATOMIC_FLAG_INIT;
std::atomic<const char*> stage{"before E011 startup"};
std::atomic<bool> townNativeValid{false};
std::atomic<unsigned> townNativeOpcode{0};
std::atomic<unsigned> townNativeAddress{0};
std::atomic<unsigned> townNativeArgc{0};
std::atomic<unsigned> townNativeDispatcherNext{0};
std::atomic<unsigned> townNativeAuthoredNext{0};
std::atomic<bool> townScriptFaultValid{false};
std::atomic<unsigned> townScriptFaultAddress{0};
std::atomic<unsigned> townScriptFaultOpcode{0};
std::atomic<int> townScriptFaultResult{0};
#if LAGI_HAS_UNWIND
struct Trace { std::uintptr_t addresses[32]{}; unsigned count{}; };
_Unwind_Reason_Code collect(_Unwind_Context* context,void* opaque)
{
    auto& trace=*static_cast<Trace*>(opaque);
    if(trace.count==32)return _URC_END_OF_STACK;
    trace.addresses[trace.count++]=_Unwind_GetIP(context);
    return _URC_NO_REASON;
}
#endif
void report(const char* reason,const char* detail,void* caller,const char* context=nullptr)
{
    if(reporting.test_and_set())return;
    // Separate append-only file: never race the normal log writer's file offset,
    // wait for its queue mutex, allocate a heap buffer, or suppress termination.
    const int fd=sceIoOpen("ux0:data/lagi/lagi-fatal.log",
        SCE_O_WRONLY|SCE_O_CREAT|SCE_O_APPEND,0666);
    if(fd>=0) {
        char buffer[512];
        int n=std::snprintf(buffer,sizeof(buffer),
            "\n[Fatal] kind=%s type=%.96s thread=%08X caller=%p imageAbort=%p lastE011Stage=%.128s\n",
            reason,detail?detail:"none",sceKernelGetThreadId(),caller,
            reinterpret_cast<void*>(&__wrap_abort),stage.load(std::memory_order_relaxed));
        if(n>0)sceIoWrite(fd,buffer,n<int(sizeof(buffer))?n:int(sizeof(buffer))-1);
        if(context) {
            n=std::snprintf(buffer,sizeof(buffer),"[FatalAssert] %.480s\n",context);
            if(n>0)sceIoWrite(fd,buffer,n<int(sizeof(buffer))?n:int(sizeof(buffer))-1);
        }
        if(townNativeValid.load(std::memory_order_acquire)) {
            n=std::snprintf(buffer,sizeof(buffer),
                "[FatalTownNative] opcode=%u at=%08X argc=%u returned=%08X authored=%08X\n",
                townNativeOpcode.load(std::memory_order_relaxed),
                townNativeAddress.load(std::memory_order_relaxed),
                townNativeArgc.load(std::memory_order_relaxed),
                townNativeDispatcherNext.load(std::memory_order_relaxed),
                townNativeAuthoredNext.load(std::memory_order_relaxed));
            if(n>0)sceIoWrite(fd,buffer,n<int(sizeof(buffer))?n:int(sizeof(buffer))-1);
        }
        if(townScriptFaultValid.load(std::memory_order_acquire)) {
            n=std::snprintf(buffer,sizeof(buffer),
                "[FatalTownScript] saturnEA=%08X opcode=%02X result=%d\n",
                townScriptFaultAddress.load(std::memory_order_relaxed),
                townScriptFaultOpcode.load(std::memory_order_relaxed),
                townScriptFaultResult.load(std::memory_order_relaxed));
            if(n>0)sceIoWrite(fd,buffer,n<int(sizeof(buffer))?n:int(sizeof(buffer))-1);
        }
        // Caller is written before unwinding: missing EH tables must not lose it.
        #if LAGI_HAS_UNWIND
        Trace trace;
        _Unwind_Backtrace(collect,&trace);
        for(unsigned i=0;i<trace.count;++i) {
            n=std::snprintf(buffer,sizeof(buffer),"[Fatal] frame=%u pc=%08X\n",
                i,static_cast<unsigned>(trace.addresses[i]));
            if(n>0)sceIoWrite(fd,buffer,n);
        }
        #endif
        sceIoClose(fd);
    }
    reporting.clear();
}
}

extern "C" void lagiSetE011FatalStage(const char* value) { stage.store(value,std::memory_order_relaxed); }
extern "C" void lagiSetTownScriptNativeContext(unsigned opcode,unsigned address,
    unsigned argc,unsigned returned,unsigned authored)
{
    townNativeOpcode.store(opcode,std::memory_order_relaxed);
    townNativeAddress.store(address,std::memory_order_relaxed);
    townNativeArgc.store(argc,std::memory_order_relaxed);
    townNativeDispatcherNext.store(returned,std::memory_order_relaxed);
    townNativeAuthoredNext.store(authored,std::memory_order_relaxed);
    townNativeValid.store(true,std::memory_order_release);
}
extern "C" void lagiSetTownScriptFatalContext(unsigned address,unsigned opcode,int result)
{
    townScriptFaultAddress.store(address,std::memory_order_relaxed);
    townScriptFaultOpcode.store(opcode,std::memory_order_relaxed);
    townScriptFaultResult.store(result,std::memory_order_relaxed);
    townScriptFaultValid.store(true,std::memory_order_release);
}
extern "C" [[noreturn]] void __real_abort();
extern "C" [[noreturn]] void __wrap_abort()
{
    report("abort",nullptr,__builtin_return_address(0));
    __real_abort();
}
extern "C" [[noreturn]] void __real___cxa_throw(void*,std::type_info*,void(*)(void*));
extern "C" [[noreturn]] void __real___assert_func(const char*,int,const char*,const char*);
extern "C" [[noreturn]] void __wrap___assert_func(const char* file,int line,const char* function,const char* condition)
{
    char context[480];
    std::snprintf(context,sizeof(context),"file=%.128s line=%d function=%.128s condition=%.128s",
        file?file:"none",line,function?function:"none",condition?condition:"none");
    report("c-assert",condition,__builtin_return_address(0),context);
    __real___assert_func(file,line,function,condition);
}
extern "C" [[noreturn]] void __real__ZSt21__glibcxx_assert_failPKciS0_S0_(const char*,int,const char*,const char*);
extern "C" [[noreturn]] void __wrap__ZSt21__glibcxx_assert_failPKciS0_S0_(const char* file,int line,const char* function,const char* condition)
{
    char context[480];
    std::snprintf(context,sizeof(context),"file=%.128s line=%d function=%.128s condition=%.128s",
        file?file:"none",line,function?function:"none",condition?condition:"none");
    report("cxx-assert",condition,__builtin_return_address(0),context);
    __real__ZSt21__glibcxx_assert_failPKciS0_S0_(file,line,function,condition);
}
extern "C" [[noreturn]] void __wrap___cxa_throw(void* object,std::type_info* type,void(*destroy)(void*))
{
    report("cxx-throw",type?type->name():nullptr,__builtin_return_address(0));
    __real___cxa_throw(object,type,destroy);
}

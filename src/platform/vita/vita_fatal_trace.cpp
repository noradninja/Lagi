// Fatal-only diagnostics bypass the asynchronous log queue. A partial Vita
// core can end before every memory segment, including the main-thread stack.
#include <psp2/io/fcntl.h>
#include <psp2/kernel/threadmgr.h>
#include <unwind.h>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <typeinfo>
extern "C" [[noreturn]] void __wrap_abort();

namespace {
std::atomic_flag reporting=ATOMIC_FLAG_INIT;
std::atomic<const char*> stage{"before E011 startup"};
struct Trace { std::uintptr_t addresses[32]{}; unsigned count{}; };
_Unwind_Reason_Code collect(_Unwind_Context* context,void* opaque)
{
    auto& trace=*static_cast<Trace*>(opaque);
    if(trace.count==32)return _URC_END_OF_STACK;
    trace.addresses[trace.count++]=_Unwind_GetIP(context);
    return _URC_NO_REASON;
}
void report(const char* reason,const char* detail,void* caller)
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
        // Caller is written before unwinding: missing EH tables must not lose it.
        Trace trace;
        _Unwind_Backtrace(collect,&trace);
        for(unsigned i=0;i<trace.count;++i) {
            n=std::snprintf(buffer,sizeof(buffer),"[Fatal] frame=%u pc=%08X\n",
                i,static_cast<unsigned>(trace.addresses[i]));
            if(n>0)sceIoWrite(fd,buffer,n);
        }
        sceIoClose(fd);
    }
    reporting.clear();
}
}

extern "C" void lagiSetE011FatalStage(const char* value) { stage.store(value,std::memory_order_relaxed); }
extern "C" [[noreturn]] void __real_abort();
extern "C" [[noreturn]] void __wrap_abort()
{
    report("abort",nullptr,__builtin_return_address(0));
    __real_abort();
}
extern "C" [[noreturn]] void __real___cxa_throw(void*,std::type_info*,void(*)(void*));
extern "C" [[noreturn]] void __wrap___cxa_throw(void* object,std::type_info* type,void(*destroy)(void*))
{
    report("cxx-throw",type?type->name():nullptr,__builtin_return_address(0));
    __real___cxa_throw(object,type,destroy);
}

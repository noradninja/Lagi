#include <cassert>
#include <cstdio>
#include <cstdint>
static unsigned fakeVcount=0,waits=0;
static unsigned sceDisplayGetVcount(){return fakeVcount;}
static void sceDisplayWaitVblankStart(){++fakeVcount;++waits;}
static bool g_presentClockInitialized=false;
static unsigned g_lastPresentVcount=0,g_pendingNativeFrameVblanks=2;
namespace logging {template<class... Args>void writef(const char*,Args...) {}}
#include "menu_cadence_clock_production.inc"
#include "menu_cadence_setter_production.inc"
using u8=std::uint8_t;
struct {unsigned m14_numVsyncPerFrame=2;} vblankData;
struct s_mainMenuWorkArea {signed char m1=0;static void Delete(s_mainMenuWorkArea*);};
#include "menu_cadence_delete_production.inc"
static unsigned frame(unsigned requested,unsigned renderTicks)
{
    presentation_set_native_frame_vblanks(requested);
    const unsigned before=fakeVcount;
    fakeVcount+=renderTicks;
    waitForPresentSlot(g_pendingNativeFrameVblanks);
    // NEXTFRAME scanout and the existing post-queue wait consume one tick.
    sceDisplayWaitVblankStart();markPresented();
    return fakeVcount-before;
}
int main()
{
    assert(frame(2,0)==2); // Gameplay, including initialized first frame.
    assert(frame(2,0)==2);
    assert(frame(1,0)==1); // Native status menu, no inserted pre-queue tick.
    for(unsigned i=0;i<16;++i)assert(frame(1,0)==1);
    assert(frame(2,0)==2); // Native Delete restores gameplay cadence.
    assert(frame(2,1)==2); // Rendering already consumed the pre-queue tick.
    assert(frame(2,2)==3); // Slow rendering is not hidden by extra simulation.
    assert(frame(1,1)==2); // Cannot promise60Hz when rendering misses budget.
    assert(frame(0,0)==2 && frame(3,0)==2 && frame(0xFFFFFFFF,0)==2);
    fakeVcount=0;g_lastPresentVcount=0xFFFFFFFF;g_presentClockInitialized=true;
    const unsigned priorWaits=waits;
    waitForPresentSlot(2);assert(waits==priorWaits); // Unsigned display counter wrap.
    presentation_set_native_frame_vblanks(1);
    g_lastPresentVcount=fakeVcount;waitFor30HzPresentSlot();
    assert(fakeVcount==1); // Movie wrapper remains fixed2 even during menu cadence1.
    for(unsigned saved=0;saved<=2;++saved) {
        s_mainMenuWorkArea menu;menu.m1=static_cast<signed char>(saved);
        vblankData.m14_numVsyncPerFrame=1;s_mainMenuWorkArea::Delete(&menu);
        assert(vblankData.m14_numVsyncPerFrame==saved);
    }
    std::puts("PASS: production display pacing1/2, rendering overruns, counter wrap, movie wrapper isolation and native menu Delete restore (mock display; not hardware timing)");
}

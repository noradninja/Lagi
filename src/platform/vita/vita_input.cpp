#include "lagi/platform.h"
#include <psp2/ctrl.h>
namespace lagi::platform::input {
static bool g_exit=false;
bool init(){ sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG); return true; }
void update(){ SceCtrlData pad{}; sceCtrlPeekBufferPositive(0,&pad,1); if((pad.buttons&SCE_CTRL_START)&&(pad.buttons&SCE_CTRL_SELECT)) g_exit=true; }
bool exit_requested(){ return g_exit; }
}

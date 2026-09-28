#include <psp2/kernel/processmgr.h>
#include <cstdio>
#include "lagi/platform.h"
int main() {
 std::printf("Lagi - native PDS runtime for PlayStation Vita\n");
 if (!lagi::platform::init()) { std::printf("Platform initialization failed.\n"); sceKernelExitProcess(1); return 1; }
 std::printf("Platform initialized.\nData root: %s\nPDS data: %s\n", lagi::platform::filesystem::data_root(), lagi::platform::filesystem::game_data_present() ? "present" : "not detected");
 std::printf("Azel runtime: not linked yet (bootstrap stage)\n");
 while (lagi::platform::running()) { lagi::platform::begin_frame(); lagi::platform::end_frame(); }
 lagi::platform::shutdown(); sceKernelExitProcess(0); return 0;
}

#include <psp2/kernel/processmgr.h>
#include <cstdio>
#include "lagi/platform.h"
#include "lagi/azel_runtime.h"
int main() {
 std::printf("Lagi - native PDS runtime for PlayStation Vita\n");
 if (!lagi::platform::init()) { std::printf("Platform initialization failed.\n"); sceKernelExitProcess(1); return 1; }
 std::printf("Platform initialized.\nData root: %s\nPDS data: %s\n", lagi::platform::filesystem::data_root(), lagi::platform::filesystem::game_data_present() ? "present" : "not detected");
 lagi::platform::renderer::status("[PASS] VITA FRAMEBUFFER / PLATFORM", 0xFF30E030u);
 if (!lagi::azel::runtime_smoke_init()) { std::printf("Azel runtime smoke init failed.\n"); lagi::platform::shutdown(); sceKernelExitProcess(2); return 2; }
 std::printf("Azel runtime: native task smoke test active\n");
 while (lagi::platform::running()) { lagi::platform::begin_frame(); lagi::azel::runtime_smoke_frame(); lagi::platform::end_frame(); }
 lagi::platform::shutdown(); sceKernelExitProcess(0); return 0;
}

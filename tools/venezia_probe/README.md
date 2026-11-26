# Standalone Venezia probe (Step 3A)

Not linked into Lagi. Dedicated title ID LAGIVNZ01. Uses manually installed VVNZRunner v2.1, **not** packaged in VPK. No changes to taiHEN configuration or clocks. Strong import is intentional in this separate test VPK: plugin absent/incompatible may prevent launch. Upstream uses fixed firmware offsets; verify firmware compatibility before installing or executing. Do not test on a device with unknown compatibility or other active codec clients. Plugin may patch global codec firmware at module start; do not dynamically unload it.

External inputs:
- https://github.com/GrapheneCt/VVNZRunner/releases/tag/v2.1 — VVNZRunner.skprx and vitasdk_stubs.zip (not stubs.zip, which targets official SDK).
- Clone VVNZRunner separately and checkout tag v2.1 (c1b777ad4af230947a6732e8b17295e5f7ed9d41). Do not mix release stubs and main branch CRT/headers.
- mep-elf-gcc and mep-elf-readelf from https://github.com/TeamMolecule/mepsdk ; source-only, legacy GCC 6.3/binutils 2.27, no verified Windows binary/toolchain build here. Use existing compiler or Linux/WSL build; upstream requires Unix make, dependencies, older CMake compatibility, and reachable verified downloads. Do not assume VitaSDK provides MeP GCC.
- Python 3, normal VitaSDK ARM cross tools and Ninja.

From Linux/WSL with MeP compiler in PATH:
```sh
bash tools/venezia_probe/build_mep.sh /path/to/VVNZRunner /path/to/mep_probe.elf
```
Script uses original v2.1 CRT from local checkout and original project-owned kernel. It rejects unresolved/symbolic movu operands and unsafe ELF layout. If validation fails, inspect generated assembly/linker layout before hardware execution; do not disable the check. A 512-byte region at file offset 256 is overwritten by the bridge, so it must not contain any section or header table. No external SDK code vendored.

From PowerShell at Lagi root, adjust stub path to actual extracted VitaSDK archive:
```powershell
$env:VITASDK = "E:\dev\VitaSDK"
$env:PATH = "$env:VITASDK\bin;$env:PATH"
cmake -S tools/venezia_probe -B build-venezia-probe -G Ninja `
  -DCMAKE_BUILD_TYPE=Release `
  "-DVVNZRUNNER_STUB=E:/dev/VVNZRunner-stubs/libVVNZRunner_stub.a" `
  -DMEP_ELF=E:/dev/mep_probe.elf
cmake --build build-venezia-probe --target VeneziaProbe.vpk-vpk
```
Confirm exact stub filename from ZIP listing. No PSP2CGC or Lagi CMake flags required. ARM uses three minimal v2.1 C ABI declarations instead of the upstream official-SDK-specific header; the original sample CRT and headers remain external MeP build inputs.

Install only on known-compatible test unit after plugin setup/reboot. Launch probe, wait for launch, press Cross once to start (Circle cancels). Screen is intentionally unrendered so no CDRAM allocations precede ELF; log is ux0:data/lagi/venezia_probe.log. Probe exits after run. Log flushed before injection, first guarded RPC and restore so last operation is visible on failure. If stuck or failed after injection, reboot before retry; no automatic cancellation/reinjection or module unload. Run twice from fresh launches only after PASS. Return full log and report firmware, plugin version and device type.

Probe cases: cold echo, 512 warm echo calls, 512 x 256-round unsigned deterministic math, 512 x 4096-round math, invalid command and excessive rounds. Full packet bytewise comparison against ARM, guard and serial; p50/p95/p99/max microseconds per blocking RPC, excluding ARM reference computation and logging. No V-thread worker or mapped large buffer yet. This isolates RPC before Step 3B. No hardware result or build success claimed until measured.

Host smoke test (any C99 compiler): cc -std=c99 -Wall -Wextra host_test.c -o host_test && ./host_test

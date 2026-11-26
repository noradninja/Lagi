# Step 3 — Venezia bridge feasibility

Stable ARM baseline: 1e84d7e (main). Experiment only on feature/venezia-audio-offload. No gameplay audio changes in this checkpoint.

## Upstream audit

VVNZRunner release v2.1: https://github.com/GrapheneCt/VVNZRunner/releases/tag/v2.1 (tag commit c1b777ad4af230947a6732e8b17295e5f7ed9d41). Assets: VVNZRunner.skprx, vitasdk_stubs.zip, stubs.zip. Main source audited at 62c46c234212346f1309b6463d1d062c5d762245; not identical to release tag. Relevant restrictions checked in tagged main.c/common.h/header. Use one consistent release/header/stub set; record binary hashes before testing. No license identified in upstream repository metadata/tree; public availability is not redistribution permission. Do not bundle until clarified.

- vnzBridgeInject accepts ELF in base CDRAM memblock, size multiple of 256 KiB, mapped Venezia physical address must be 0x20000000. Allocate code before renderer/other CDRAM consumers; reserve first block only in an explicitly enabled experiment. One registered PID; do not assume independent injection sessions. Validate ELF architecture, little endian, entry, bounds, and communication region layout before calling plugin. Inspect failed-inject cleanup; upstream error path does not fully unwind user map. Keep probe isolated.
- vnzBridgeExec is synchronous/blocking with process suspend guard; max user argument 500 bytes (512-byte comm heap minus 12-byte header). Use fixed-width integer ABI, version and lengths; no ARM pointers in packet. No use of NoSuspend path in initial experiment. Do not assume a timeout can cancel a stuck RPC safely.
- MapMemory requires 256-byte aligned memory; use dedicated aligned spans and mapped Venezia address, then MeP memory ownership open/close operations. PUBLIC/PRIVATE/PROTECTED are distinct; validate visibility in both directions. Unmap RW invalidates ARM cache in source; persistent mapping coherence across repeated ARM/MeP ownership transitions must be measured, not assumed.
- vnzBridgeRestore resets Venezia and releases mapped ELF; do not release code before restore completes. Module startup patches codec firmware and uses fixed firmware-dependent offsets; module_stop is effectively empty. Do not dynamically unload or auto-install plugin in this stage. No verified firmware compatibility table; require explicit known-compatible test device, check firmware first. Avoid concurrent video/audio hardware codec clients, and check movie/codec coexistence separately.
- README says up to six V-thread worker cores normally, seven extended. Calls still block on ARM. Start one core, default 166 MHz clock; measure rather than change clocks.

MeP toolchain upstream https://github.com/TeamMolecule/mepsdk pinned dd639d89078ac9d1238e06068edfcde384947f33. Upstream sample Makefile uses mep-elf-gcc, freestanding flags, -mivc2 and -O3; includes a special assembly transformation for symbol addresses and GET_SYMBOL_ADDR. Audit actual linker layout, section/entry validation, reserved 512-byte comm heap at file offset 256, helper ABI and compiler output before running. No toolchain build or firmware test performed yet.

## 3A — isolated probe (first implementation)

Opt-in standalone Vita VPK, not linked into normal Lagi. External plugin/stubs supplied locally by tester (no third-party binary redistribution). Confirm working firmware and installed plugin before launching probe. Build freestanding MeP ELF with one versioned packet and deterministic operations. Use guarded Exec only. Verify known input/output, size limits, malformed command handling, repeated warm invocations, clean restore/relaunch. Keep code memblock allocated until restore. Report all error codes and timings. ARM baseline stays untouched. If firmware compatibility or toolchain cannot be established, stop before injection.

## 3B — transfer and memory coherency

Compare packet-only no-op/echo, aligned dedicated mapped RW buffers and explicit map/unmap per call. Use patterned buffers, generation counters, guard regions, checksums plus bytewise comparison; ensure no concurrent access. Sizes 1 KiB (256 stereo int16 frames), 4, 16, 64, 512 KiB (SCSP RAM scale). Measure cold init separately, warm no-op, ownership transitions, data transfer, kernel math and total round trip. Report n, p50/p95/p99/max and failure count; no logging inside timed loop. Repeat with graphics contention and suspend/resume only after isolated probe is proven.

## 3C — budget gate

Audio quantum 256 samples at 44.1 kHz = 5.805 ms. At least 172.3 dispatches/s if one dispatch per quantum. Existing compiled ARM DSP typically 1.5–1.8 ms; bridge + transfers + MeP computation must beat matching ARM reference, including contention/tail latency. Initial engineering target warm dispatch p95 <=0.25 ms (aspirational), no correctness failures; compare 256/512/1024-frame work units while preserving latency and sample order. Do not use per-sample RPC (44,100/s). No audio queue changes to hide slow bridge.

## 3D — architecture decision after measurements

SCSP DSP consumes MIXS from slots each sample and mutates shared sound RAM/state, with returns used in output mixing; M68K register/RAM accesses interleave with it. A block-only DSP offload requires proof that producing inputs ahead of time does not change observable state, or move the interdependent loop to MeP. Prototype offline deterministic DSP replay first using captured programs; only integrate real audio after equivalence and total cost are established. If isolated DSP is not safe/profitable, evaluate combined SCSP/M68K quantum on Venezia. Keep PCM output, game command queue, and disc I/O preparation ARM-owned. No production backend switch until hardware validation and optional dependency handling.

No kernel module installation, third-party code copying, audio changes or hardware execution in this documentation commit.

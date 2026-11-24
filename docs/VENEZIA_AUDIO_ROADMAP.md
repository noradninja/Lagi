# Venezia audio offload roadmap

Development branch: feature/venezia-audio-offload. Baseline: af2b0791976ba53fc28f5c70e8770be2274f19cf. Preserve the existing ARM backend, Azel timing, renderer and audio ownership semantics. Atomic commits; developer pulls, builds and validates on Vita hardware.

## 1. Baseline
Consumer instrumentation 136b83c reports [LagiPCM] missing frames, empty/partial blocks and queue low water. Startup and intentional finished-stream padding are excluded. Counters are per output thread lifetime. Worker instrumentation 4f100cf reports [AzelAudioWorker] sampled active iteration time including events, commands, DSP service, output lease and rendering/mixing/submission. gapUs is the inter-iteration gap, not a complete scheduler latency measurement; deliberate sleep is excluded. Samples are sparse and may miss spikes; logging itself can perturb timing. Correlate with [AzelAudioPerf]. No hardware or compile validation yet.

44100 Hz, 256-frame synthesis quantum = 5804.99 us; 1024-frame output block = 23.22 ms; target queue 2048 frames = 46.44 ms. Historical 84-step ARM DSP median 2268 us, synthesis median 5297 us (see SCSP_DSP_TRANSLATOR.md); not a fresh measurement. Test camp, field, battle, Cinepak transitions and CDDA. Record build flags, backend, clock settings and log. Do not change buffer sizes until starvation is characterized.

## 2. Bridge feasibility
Upstream: https://github.com/GrapheneCt/VVNZRunner
Inspect pinned source and release artifacts, license and firmware requirements. Measure injection, blocking execution, cache ownership transitions, mapped buffer transfer, warm/cold dispatch, contention and suspend/resume. Use coarse work units, never per-sample bridge calls. Verify ABI and alignment; ARM pointers are not MeP pointers. No assumptions about cache coherence or speed.

## 3. DSP prototype
Compile existing generated straight-line C DSP code with MeP toolchain before investing in a machine-code emitter. Use captured 52/84-step and synthetic 128-step programs. Compare state, output and sound RAM writes against reference, including overflow, addressing and program changes. Benchmark complete round trip, not only execution. Zero correctness mismatches required. Target synthesis under 4 ms per quantum including overhead; aspirational, not measured.

## 4. Combined sound core
Investigate portable 68000 and SCSP core together on Venezia to preserve per-sample CPU/DSP interleaving, RAM and IRQ semantics. Keep ARM control plane and PCM output. Define explicit ownership and synchronization for sound RAM, bank loading, command queues, PCM and state. Validate sequence changes, battle sounds, fades, suspend, movies and shutdown. ARM remains default until acceptance. Fallback before starting a stream; live fallback requires validated state transfer.

## 5. Deployment
Goal: no manual dependency installation. Investigate bundling pinned VVNZRunner kernel module and MeP ELF in VPK, conditional on redistribution permission. Prefer detection of compatible existing module and optional runtime loading through supported taiHEN facilities. Verify optional import resolution before linking a mandatory dependency. Verify firmware compatibility, privileges, module ownership, lifecycle, failure handling and suspend safety. Never unload another application's module. Never silently modify taiHEN configuration. If persistent installation is necessary, require explicit consent, backup, atomic config update, duplicate detection, rollback and clear reboot instructions. Do not bundle or install until license and lifecycle are verified. Unsupported devices or failed initialization use ARM.

## Acceptance
Zero steady-state missing PCM frames in representative gameplay, no correctness mismatches, no gameplay/render regressions, no movie/CDDA ownership regressions. Test handheld Vita and PSTV, plugin absent/present/incompatible, cold launch, relaunch and suspend/resume. Document hardware results and exact commits before enabling optional backend by default.

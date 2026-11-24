# Venezia audio offload roadmap

Development branch: feature/venezia-audio-offload. Stable merged baseline: 84293e4e5b8af2dfbf3f88b1bfcbfc747f025563; synchronized feature checkpoint: 37f117c83f46331ec7e3ded0969e061c0b18ad9f. Preserve the existing ARM backend, Azel timing, renderer and audio ownership semantics. Atomic commits; developer pulls, builds and validates on Vita hardware.

## 1. Baseline
Consumer instrumentation cbee8b9 restores [LagiPCM] on the feature branch after synchronization with main. Each stream has a serial, sample rate, channel count, block size and monotonic timestamp. Cumulative steadyBlocks, underrunBlocks, emptyBlocks and missingFrames count successfully submitted blocks after startup, excluding finished-stream padding. minQueuedFrames is interval-local. Reports every 128 eligible blocks plus a final partial interval on clean thread exit. Separate streams; difference cumulative counters within a stream, never across restarts. Abrupt termination can lose the last partial interval. This measures software PCM starvation, not every possible hardware or audible discontinuity.

Worker instrumentation 4f100cf reports [AzelAudioWorker] sampled active iteration time including events, commands, DSP service, output lease and rendering/mixing/submission. gapUs is the inter-iteration gap, not a complete scheduler latency measurement; deliberate sleep is excluded. Timestamp commit e601455 adds tUs to sampled worker and synthesis reports. Samples are sparse and may miss spikes; logging itself can perturb timing. Correlate with [AzelAudioPerf]. No compile or hardware validation of the restored instrumentation yet.

44100 Hz, 256-frame synthesis quantum = 5804.99 us; 1024-frame output block = 23.22 ms; target queue 2048 frames = 46.44 ms. Historical 84-step ARM DSP median 2268 us, synthesis median 5297 us (see SCSP_DSP_TRANSLATOR.md); not a fresh measurement. Do not change buffer sizes or scheduling until starvation is characterized.

### Natural-play workload protocol
Identical gameplay replays are not required. Use the same device, clocks, build configuration and ARM backend when practical; record deviations. Play normally across available representative workloads: idle/moving Camp, field traversal, ordinary and busy battles, and transitions (including Cinepak/CDDA when naturally encountered). Aim for roughly 1–2 minutes per available steady workload, preferably with repeat visits; shorter captures are useful and no exact route or timing is required. Exit normally to flush the final interval. Retain full logs and note device, clock overrides, locations and audible issues. Do not require artificial pauses or repeated identical encounters.

### Analysis
- Group output intervals by stream, rate and scene/sequence when identifiable from neighboring log events; mark mixed or ambiguous intervals separately. Treat startup/load/ownership transitions as their own workload rather than hiding them in steady-state averages.
- Normalize missing frames by steadyBlocks * blockFrames (percent or ppm); report underrun blocks per 1000 eligible blocks, empty blocks separately, and queue low-water distributions. Use counter deltas within each stream and weight by observed eligible frames, not number of reports.
- Stratify sampled synthesis cost by sequence, DSP hash/steps/backend and active-slot ranges. Report sample count, median, P95, maximum and fraction exceeding the quantum deadline; do not treat sparse samples as exhaustive. m68k/SCSP component estimates are nested/interleaved, not additive independent wall-clock times.
- Compare worker active time, rendered vs non-rendered samples, lease time and gaps; correlate with PCM reports and existing scene/render timing. Timestamped audio records support temporal correlation, but asynchronous log ordering and sparse samples cannot prove exact per-underrun causation.
- Compare representative distributions and worst cases across workloads, not raw total underruns across different playthroughs. Preserve outliers and note sample sizes. Capture the baseline before implementing MeP execution.

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

# SCSP DSP fallback and sustained PCM underruns — 20 November 2026

**Hardware checkpoint:** `diagnostics/audio-queue-starvation` at `44da952`  
**Capture:** `lagi(20261120-171445).log` (20,308 lines, 2,383,647 bytes; Vita hardware, Disc 1)  
**Scope:** Native Saturn sound driver (M68K + SCSP voices/DSP + Vita output). The added title CD-DA lane is not the primary workload under investigation.

## Where we stand

We have a measurable native-audio throughput problem, not an intentionally reduced playback sample rate. The audio output uses 44,100 Hz and continues submitting 1,024-frame blocks, but the PCM producer falls behind and the output thread pads missing source frames with zeros. In demanding sound sequences this happens continuously, not only at scene boundaries.

The most actionable finding is **loss of the generated ARM DSP execution path for a recurring 52-step game program**. The game program is found and compiled successfully; its cache is hit again on later loads. Despite that, most sampled steady-state execution uses the generic reference DSP interpreter. This increases the work per 256-frame quantum beyond the 5,804-microsecond real-time budget in busy scenes.

**What we have proven:** the fallback is occurring, the output is underrunning, and the reference path is substantially slower in the captured workloads.

**What is still a hypothesis:** a write to SCSP DSP microprogram memory (MPRO) invalidates the native function after a successful start, and a later reselection never follows. The log cannot yet identify the write or prove whether its contents change. We must trace that transition before changing the dirty-state rules.

This is a research and implementation handoff for the next audio session, not a claim that the problem has been fixed.

## The complete output deficit

`[AzelAudioQueue]` provides per-window quantum times and submitted-output starvation counters. There are 316 queue records overall; the six `seq=-1` records describe movie/title handoff rather than a named native sound sequence. The remaining 310 records cover the native audio sections below.

| Sequence ID | Reporting windows | Quanta | Weighted mean / 256 frames | Submitted blocks | Underrun blocks | Underrun blocks / submitted | Zero-filled PCM frames |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 75 | 8 | 1,396 | 2,053 µs | 347 | 1 | 0.3% | 1,024 |
| 64 | 24 | 4,623 | 4,029 µs | 1,287 | 132 | 10.3% | 134,400 |
| 1 | 17 | 3,247 | 4,604 µs | 980 | 168 | 17.1% | 171,520 |
| 4 | 27 | 5,165 | 6,770 µs | 1,524 | 820 | 53.8% | 237,056 |
| 2 | 36 | 6,893 | 6,507 µs | 1,981 | 953 | 48.1% | 265,728 |
| 3 | 122 | 23,397 | 7,085 µs | 7,290 | 5,094 | 69.9% | 1,475,328 |
| 8 | 31 | 5,912 | 6,707 µs | 1,726 | 931 | 53.9% | 253,696 |
| 7 | 45 | 8,453 | 6,025 µs | 2,531 | 1,103 | 43.6% | 427,008 |
| **Total** | **310** | **59,086** | — | **17,666** | **9,202** | **52.1%** | **2,965,760** |

Calculation: group the `[AzelAudioQueue]` lines by `seq`; add interval counters, and weight `renderAvgUs` by `quanta`. The source reports integer-rounded per-window means, so the grouped means are approximations. The window label can straddle a sequence or output-owner transition and is not a song identity. The counters represent *missing produced samples* padded by the output service; they do not measure deliberately quiet music or muted SCSP voices.

At 44,100 frames/s, the zero-fill total represents **67.25 seconds of missing source PCM**, distributed across about **410.20 seconds of submitted native output blocks**. That is **16.39% of output frames**. It does not mean one continuous 67-second silence; many blocks contain only partial padding.

The target is 256 frames at 44.1 kHz, or **5,804 µs of audio time**. Sequence 3's 7,085-µs mean cannot maintain real-time output indefinitely. The same is true of the sustained 6.0–6.8-ms means in other active sequences. Increasing the queue can absorb occasional spikes but cannot fix this persistent deficit.

The legacy `shortWrites` counter remains zero in the sampled `[AzelAudioPerf]` records. It counts producer-side ring writes, *not* hardware submissions that arrive too late. The new zero-fill counters are the relevant evidence.

## The recurring 52-step program

The largest mismatch is program **`35C2F2CD`, `LastStep=52`**, with 4,748 bytes of generated ARM code. The log shows a successful `arm-compiled` event followed by multiple `arm-hit` events for the same hash. That verifies cache lookup and native-code generation, not sustained execution.

The 390 timed `[AzelAudioPerf]` quantum samples contain the following DSP program families:

| Program hash | Steps | Sampled quanta | Observed sample backend |
| --- | ---: | ---: | --- |
| `050C5D1F` | 0 | 54 | Mostly reference/stopped path; not a comparable nonempty DSP workload |
| `D606D1D9` | 48 | 18 | 18 fully ARM |
| **`35C2F2CD`** | **52** | **288** | **260 fully reference; 28 fully ARM** |
| `51E22190` | 62 | 30 | 30 fully ARM |

The `52-step` breakdown is unusually clear: `260/288 = 90.3%` of the observed quanta execute the reference interpreter, while only `28/288 = 9.7%` execute native ARM. All 260 reference records have `dspBackendSamples=32,0,0,0`; all 28 ARM records have `0,0,0,32`. This is not an average of mixed backends within those sampled quanta.

| 52-step measurements | Native ARM (28) | Reference (260) |
| --- | ---: | ---: |
| Median DSP time | 1,580 µs | 3,312 µs |
| Median whole quantum | 5,197 µs | 7,443 µs |
| Median M68K estimate | 1,904 µs | 1,944 µs |
| Median active voices | 18 | 21.5 |
| Budget | 5,804 µs | 5,804 µs |

The two sets have different voice counts and game conditions: this is an observational comparison, not a controlled matched-voice benchmark. Even so, the approximately **1.73-ms DSP median difference** is larger than the limited remaining headroom, and the reference median quantum is approximately **1.64 ms over budget**.

The 48- and 62-step programs are successfully using ARM in every sampled quantum in this capture. This argues against a blanket failure of Vita executable memory or the ARM emitter. We need to explain why the 52-step program takes a different path.

### Concrete log anchors

- **Lines 1072 and 1082:** `35C2F2CD` is first queued (`reason=arm-pending`), then successfully built (`reason=arm-compiled`, 4,748 bytes).
- **Lines 2528–2540:** a sequence of 52-step samples runs on ARM with ~5-ms total quanta; lines 2543 and 2547 then report fresh `arm-hit` events.
- **Lines 2549–2550:** the next visible timed 52-step sample reports `dspBackend=0`, all 32 sampled frames on the reference interpreter, total 7,491 µs, DSP 3,448 µs.
- **Lines 3298–3303:** another `arm-hit` is followed by a reference-only quantum (`6,412 µs` total).
- **Lines 18121–18124:** another `arm-hit` is followed by a reference-only quantum (`6,901 µs` total).
- **Lines 18579–18580:** a subsequent cache hit occurs before further reference-only 52-step samples.

The capture does **not** have a direct record of `lagi_dsp_program_written()` or the precise backend transition. The entries above establish correlation and narrow the timing, not the cause.

## Source trace: how ARM execution can be lost

These are the current Lagi and pinned-Azel code paths, not proposed replacements.

1. **Saturn register write:** `extern/Azel/ThirdParty/aosdk/eng_ssf/scsp.c`, `SCSP_w16()`, writes `MPRO` through register offsets `0x800..0xBFF`; a write at `0xBF0` also invokes `SCSPDSP_Start()`.
2. **Generated Vita SCSP copy:** `CMakeLists.txt` (near 6651–6671) copies the upstream SCSP service and inserts `lagi_dsp_program_written(&SCSP.DSP)` before **every MPRO halfword write**. This generated copy, rather than the pinned upstream file, is what runs on Vita.
3. **Native selection:** `src/integration/lagi_scspdsp_predecoded.c`, `SCSPDSP_Start()` (near 810–824), determines `LastStep` and calls `lagi_dsp_select_native(DSP, 0)`. That routine finds a matching program by full hash, size and content; an uncached program is compiled at the next worker boundary by `lagi_dsp_service_pending()`.
4. **Invalidation:** `lagi_dsp_program_written()` (near 354–367) marks the DSP instance dirty, invalidates its decoded-state marker and clears `g_lagiNativeFunction` when it matches that instance. The current hook does not compare the old halfword against the value being written.
5. **Per-sample dispatch:** `SCSPDSP_Step()` (near 596–660) runs native ARM only with a non-null native function, the matching DSP instance, a clean dirty state, and a non-reference configured mode. Otherwise the ARM-configured build takes the generic reference interpreter. The predecoded fallback is selected only in the explicit `LAGI_DSP_PREDECODED` configuration.
6. **Rearm boundary:** `lagi_dsp_service_pending()` only services a pending DSP selected by `Start()`; an MPRO write that dirties an already-running program does not itself request another native selection.

This creates a plausible failure path: `Start()` activates ARM, a later MPRO write dirties the instance and clears its function, and the driver continues using the program without another `Start()`. But we have **not established that a late MPRO write actually occurred**. Alternatives include another global invalidation, an instance/dirty-state mismatch, a mode transition, or a sequencing issue in the generated hook and Start ordering.

A same-value write would be particularly interesting, because the current hook invalidates the native function even if the program bytes remain unchanged. That is a *testable possibility*, not permission to skip invalidation without evidence.

## Next investigation: prove the transition first

The first code change should be an **isolated, bounded diagnostic commit**. We already have output, M68K and DSP timing. We need to observe why a program that was just selected as ARM stops using its native function.

### MPRO writes and Start ordering

Capture these fields at the generated SCSP write boundary (or through a Lagi-owned callback that receives them), with the actual old/new halfword **before mutation**:

- DSP instance identity and register offset (including whether `addr==0xBF0`)
- Previous and new 16-bit values, and whether the write changes the data
- Whether the DSP was stopped or running and whether there was a valid native function
- Whether the write occurs before or after `SCSPDSP_Start()` in the relevant sound-driver sequence
- Aggregate counts of changed versus unchanged writes after `Start`, keyed by program generation and hash

Do not log every individual write to the SD card from the real-time path. Collect counters and a bounded ring of transition records, then emit a few records at a worker boundary. Distinguish control writes to COEF/MADRS from program writes to MPRO: live coefficient/address updates **must not** be treated as changes to the compiled instruction sequence.

### Native selection and fallback reasons

At `lagi_dsp_select_native()`, `lagi_dsp_program_written()`, and the first observed change of execution backend in `SCSPDSP_Step()`, record:

- Current/last-selected hash and `LastStep`
- `g_lagiNativeFunction` availability, `g_lagiNativeDsp==DSP`, dirty-state result, `DSP->Stopped` and backend mode
- Whether selection was an ARM compile, verified cache hit, deferred pending compile, code-capacity failure, publication failure, or a deliberate reference mode
- Last MPRO write location and whether a new `Start` followed it

Keep the high-frequency sample loop free of extra allocation, logging, hashing and syscalls. If an execution-path transition needs detection in that loop, use minimal in-memory state and drain it through the worker. Prefer one transition event per cause/generation, not repeated messages for every sample.

**Evidence needed to close the hypothesis:** a verified timeline showing `arm-hit/compiled → MPRO write → dirty/native function invalidated → reference backend`, and whether the microprogram contents changed. Without that timeline, we should not change the invalidation policy.

## Fix boundary, once the reason is known

This must be a semantic correction, not a blanket force-ARM switch.

- If a *same-value* MPRO write is the only trigger, preserve the original write but avoid invalidating compiled code **only when we have proved the complete instruction contents are unchanged**.
- If the driver writes genuinely changed instructions after `Start`, preserve reference execution for the dirty program until it is safe to select/compile the complete final program. The correct rearm point must be derived from the actual driver sequence, rather than a fixed timer or an arbitrary 256-frame grace period.
- If the issue is an unexpected DSP instance/state reset or a cache-reference problem, fix that ownership/lifetime issue at the source.
- If native ARM cannot cover a legitimate program state, keep the interpreter as a correct fallback and record why. Do not silently execute an outdated compiled program.

Every native-code reactivation must revalidate **hash, step count, and full instruction contents**. Keep live COEF/MADRS reads and sample state intact. Preserve the DSP's register/memory semantics, error fallback and the existing M68K-to-SCSP ordering.

The pinned Azel submodule remains untouched; make any integration changes in Lagi's generated SCSP service or replacement code, as we do today.

## Other findings to keep separate

### Multi-second sound-command delays

The new measurements also show event/dispatch delays in certain bank transitions:

| Sequence | Maximum event dequeue delay | Maximum dispatch-attempt delay |
| ---: | ---: | ---: |
| 64 | ~5.8 ms | **3,102 ms** |
| 1 | **2,889 ms** | **2,894 ms** |
| 7 | ~7.3 ms | **2,989 ms** |
| 3 | **845 ms** | **857 ms** |

These are worst individual reporting-window observations, not typical latencies and not proof of the final audible onset. The `dispatch` metric measures a submission *attempt* after the native loading-state hold; the `ack` metric covers a mailbox transaction, not a particular sound. We need to trace large delays to bank loading and/or blocked worker service before changing that flow. It is not explained by a 2,048-frame PCM target alone.

### A long worker/scheduling outlier

At log **line 19775**, sequence 7 reports `renderMaxUs=2768132` (~2.77 s) in a window with 192 quanta and substantial zero-fill. That is clearly anomalous relative to the surrounding 5–8-ms render quanta. It could reflect scheduling, blocked I/O, contention or another stall captured in a wall-time interval. The log does not say which. Investigate it separately after the persistent 52-step fallback, and preserve matched timing context rather than assuming the DSP interpreter itself ran for 2.77 seconds.

### M68K, mixer and logging

The M68K's sampled per-quantum work is typically around 1.8–2.0 ms and is broadly comparable in the ARM/reference subsets. The SCSP slot-mixer cost scales with active voices, so an optimized DSP alone does not guarantee 30 Hz game scenes or 44.1 kHz audio under every workload. Frequent field and sound-request logging is a possible contention source, but we have not isolated it experimentally.

The October 6 translator checkpoint measured an **84-step ARM DSP median of 2,268 µs** and **5,297 µs total quantum median** against **5,804 µs**. That is our historical validated reference, but it is a *different* program/workload from the recurring 52-step capture. Both comparisons matter; neither should replace the other.

## Validation gates

We can accept a native-backend fix only after all of these are true:

1. **Source reasoning:** we know exactly what invalidated the 52-step native function and why the behavior is correct after the change. The byte-level microprogram and live register state remain authoritative.
2. **Differential tests:** run `python tools/run_scspdsp_equivalence.py`, `--thumb`, `--corpus dsp_programs` with the captured game programs, and `python tools/run_scspdsp_vm_tests.py`. Extend tests to cover the actual confirmed late-write sequence, including same-value writes and genuine changed words if both occur. We must not mistake a passing synthetic corpus for Vita acceptance.
3. **Hardware backend persistence:** on the same Disc 1 scenes, steady `35C2F2CD` windows should report `dspBackendSamples=0,0,0,32` after valid selection and remain there unless the original game legitimately changes the program. Any reference fallback must carry an identified reason.
4. **Hardware throughput:** matched 52-step gameplay quanta must fall below the 5,804-µs budget consistently enough for the output to stay supplied. Capture the mean, upper tail, `renderMaxUs`, `underrunBlocks`, `zeroFillFrames` and voice counts. A speedup without reduced output starvation is not acceptance.
5. **Audio correctness:** listen across title/sequence changes, field flight, Valley and Garil battles, and menu/LCS effects. Confirm music speed, pitch, effects, stops, fades, battle sound-bank restoration and movie ownership are unchanged.
6. **Regression isolation:** keep the current diagnostics branch and the `44da952` capture baseline available for A/B builds. Change no output queue sizes, M68K clocks, synthetic waveforms or unrelated renderer systems during the native-backend repair.

Only after the DSP path is persistently native and measured again should we decide whether to tune queue startup/sustained targets. The multi-second effect delays and unusual worker stall are separate follow-up work.

## Starting point and references

- **Repository:** `noradninja/Lagi`
- **Branch:** `diagnostics/audio-queue-starvation`, HEAD `44da952` at time of capture
- **Hardware log:** `lagi(20261120-171445).log` (provided in the investigation conversation, not checked into Git; contains runtime paths and original game data identifiers)
- **Output counters:** `include/lagi/audio_output_telemetry.h` and `src/platform/vita/vita_audio.cpp`
- **M68K/audio worker:** `src/integration/lagi_audio_vita.cpp`
- **DSP translator, dirty flags and native backend:** `src/integration/lagi_scspdsp_predecoded.c`
- **Generated MPRO instrumentation:** `CMakeLists.txt`
- **Upstream program register semantics:** `extern/Azel/ThirdParty/aosdk/eng_ssf/scsp.c`
- **Previous work:** [SCSP DSP translator](SCSP_DSP_TRANSLATOR.md), [queue diagnostics](AUDIO_QUEUE_DIAGNOSTICS.md), [audio worker affinity](AUDIO_WORKER_AFFINITY.md), [Ruins DSP predecode](RUINS_DSP_PREDECODE.md)

The operating rule remains **Azel decides. Lagi services. Neptune renders.** Sound-driver ownership and program timing stay in the emulated M68K/SCSP path; the fix should remove unnecessary host overhead or incorrect invalidation without changing what the original driver plays.

# General SCSP DSP translator

Azel decides. Lagi services. Neptune renders.

Translation is keyed by complete DSP instruction contents and length, never scene names. The existing SCSPDSP interface, reference interpreter, guarded predecoded path, 256-frame quantum and worker yields remain. No extern/Azel or rendering sources are edited. CMake instruments MPRO writes in a generated Lagi-owned SCSP service copy.

## Backends and rollout

Create an ASCII file (without a BOM) at ux0:data/lagi/dsp_backend.txt containing one word, then restart Lagi:

| Value | Behavior |
| --- | --- |
| predecoded | Default diagnostic mode; current fast path, VM probe and program capture |
| reference | Generic correctness reference |
| auto | Prefer linked generated C, then gated runtime ARM; otherwise interpret |
| aot | Linked generated C only; unknown programs interpret and are captured |
| arm | Gated runtime ARM only; unavailable or failed translation interprets |

The isolated probe allocates one 1 MiB VM block, obtains its base, opens it for writing, writes a function returning 42, closes and synchronizes it, executes it and verifies the result. Every API result is logged. Predecoded, reference and AOT modes free the block after the probe. Auto and ARM modes retain and reuse that same allocation for generated code, avoiding a second Vita VM allocation. Runtime ARM is available only after the entire probe succeeds. No additional Vita plugin is required. Successful emulation does not prove executable memory works on hardware.

Runtime mode uses four 64 KiB code slots within the retained 1 MiB VM block. Start resolves existing routines or queues a translation. Compilation and publication occur at a worker boundary, outside per-sample processing; the interpreter runs until selection succeeds. No allocation or compilation occurs per sample. Code overflow or API failure falls back. Shutdown joins the audio worker before releasing routines and VM memory.

## Translation semantics

A shared field schema and decoded representation expose input, TEMP, previous ACC shifting, multiplication, register writes, memory access and effects in order. Constant instruction controls and unused calculations are folded. COEF and MADRS remain live per-instruction reads; inputs, TEMP, memory and instance state remain live. Generated functions take the current DSP instance rather than embedding its address.

Both backends preserve previous-ACC SHIFTED ordering, every shift mode, signed arithmetic, PACK/UNPACK formats, wrapped additions and addresses, odd-step memory access, read/write aliasing and effect accumulation. Explicit unsigned wrapping also removes signed-overflow undefined behavior in the reference and predecoded arithmetic.

Build-time generation emits straight-line C compiled by the existing Vita compiler. Runtime generation emits ARMv7 instructions with AAPCS register preservation and synchronized code publication. Full program contents are checked after hash matches. MPRO writes invalidate that instance; reference processing remains active until Start selects a replacement. If the SCSP writes additional MPRO words after its start-trigger write, the deferred compilation remains pending and consumes the final program image at the next worker boundary. Cache eviction, translation failure and unavailable VM preserve fallback behavior.

## Capture and local build

Loaded programs are deduplicated and saved under ux0:data/lagi/dsp_programs as .ldsp files. Captures contain instruction words, not live register or audio snapshots. Capture writes are deferred to worker boundaries; queue overflow and file failures are logged. Copy the folder contents into C:\Dev\Lagi\dsp_programs, which is ignored by Git. Full four-disc coverage requires discovering distinct programs when runtime executable memory is unavailable.

```powershell
Set-Location C:\Dev\Lagi
$env:PSP2CGC='C:\Dev\sdk\host_tools\bin\psp2cgc.exe'
$env:VITASDK='C:\Dev\VitaSDK'
$env:PATH="$env:VITASDK\bin;$env:PATH"
cmake -S . -B build -DLAGI_DSP_CORPUS=C:/Dev/Lagi/dsp_programs
cmake --build build --parallel 32
```

The link layout reserves a 16 KiB minimum gap for SCE module metadata before writable data. This avoids an SDK converter overlap failure when an empty or differently sized corpus changes the executable layout; runtime processing is unaffected.

Output: build\Lagi.vpk. Python is needed on the build machine for generation; LAGI_DSP_PYTHON can override its discovered path. No Python or emulator dependency is required on Vita. An empty corpus produces a valid build with no linked program-specific routines.

## Validation performed

Differential tests compare complete DSP state and all DSP RAM after every sample: 1,024 predecoded comparisons plus 512 sample comparisons across generic, generated C and generated ARM for 32 synthetic programs, including 52/84/128-step programs and full instruction coverage. Both ARM and Thumb helper builds passed with zero mismatches. Tests cover live changes, replacement, instances, empty/stopped programs, collisions, invalidation, eviction, deferred compilation, unavailable VM, publication failure and bounded-code overflow. Format tests cover 65,536 UNPACK and 131,072 PACK cases, saturation extrema and calling-convention preservation.

The actual platform bridge also passed emulated VM lifecycle/failure/cleanup tests; corpus parsing and deduplication tests passed. Nonempty synthetic-corpus Vita compilation and packaging passed. The three captured game programs also passed the ARM and Thumb differential suites with zero mismatches. On Vita hardware, the corrected 1 MiB VM allocation completed the full open/write/close/sync/execute lifecycle and returned 42. Runtime ARM compiled and exclusively executed the captured 52- and 84-step programs. The tester approved the ARM audio path: Disc-entry crackle is nearly eliminated, ARM is audibly better than AOT, and the quiet 2-4 second delayed effect on discrete menu/LCS sounds is accepted with this path.

PC tests require unicorn and pyelftools:

```powershell
python tools/run_scspdsp_equivalence.py
python tools/run_scspdsp_equivalence.py --thumb
python tools/run_scspdsp_equivalence.py --corpus dsp_programs
python tools/run_scspdsp_vm_tests.py
python -m unittest discover -s tests -p test_dsp_corpus.py
```

## Hardware comparison

1. Install the diagnostic/default build and run the same Ruins route. Save lagi.log before restarting, and copy the complete dsp_programs folder. Check VM probe return 42 and successful cleanup, capture errors and queue drops.
2. Run auto or arm with the same build after confirming probe success; the code still gates execution internally. If VM fails, rebuild with captured programs and select aot. Compare against predecoded using the same build and matched route, clocks and warm-up.
3. Use multiple steady-state windows with matching program hash, 84 steps, 256 frames and profiling sample count. The 2026-10-06 Vita run at commit `f7879c3` demonstrated an 84-step ARM median of 2,268 us and P95 of 2,392 us across 42 records (10,752 frames), compared with a corrected predecoded median of 5,264 us. Every timed ARM frame reported `dspBackendSamples=0,0,0,32`. This passes the <=3,000 us target. The corresponding whole-quantum median was 5,297 us, 507 us below the 5,804 us budget, with an empty-chunk delta of +1 and zero short writes.
4. Check sound and responsiveness across transitions, movies, flight and battle. Measure whole-quantum time separately against 5,804 us, and compare queue depletion counter deltas; faster DSP alone does not guarantee the audio budget.

Relevant records: LagiDSPVM (each API result), LagiDSPCapture (saved/deduplicated/error), LagiDSPProgram (hash, steps, backend, translateUs, codeBytes, sizeKind, hits, misses, reason), AzelAudioDSP and AzelAudioPerf (existing DSP/whole-quantum timing plus identity/backend). Numeric timing backend values are 0=reference, 1=predecoded, 2=aot, 3=arm; these differ from configuration enum values. `dspBackend` is latched inside the measured DSP call, and `dspBackendSamples=reference,predecoded,aot,arm` reports the count of each path across the chunk's sampled frames. A matched native run must show all samples in the selected native bucket; mixed or fallback counts invalidate the timing comparison. Preserve both log and captures for each test session. Runtime compilation time is logged separately from per-sample DSP timing.

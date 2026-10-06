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

The isolated probe allocates VM memory, obtains its base, opens it for writing, writes a function returning 42, closes and synchronizes it, executes it, verifies the result and frees it. Every API result is logged. Runtime ARM is available only after the entire probe succeeds. No additional Vita plugin is required. Successful emulation does not prove executable memory works on hardware.

Runtime mode preallocates 256 KiB: four 64 KiB code slots. Start resolves existing routines or queues a translation. Compilation and publication occur at a worker boundary, outside per-sample processing; the interpreter runs until selection succeeds. No allocation or compilation occurs per sample. Code overflow or API failure falls back. Shutdown joins the audio worker before releasing routines and VM memory.

## Translation semantics

A shared field schema and decoded representation expose input, TEMP, previous ACC shifting, multiplication, register writes, memory access and effects in order. Constant instruction controls and unused calculations are folded. COEF and MADRS remain live per-instruction reads; inputs, TEMP, memory and instance state remain live. Generated functions take the current DSP instance rather than embedding its address.

Both backends preserve previous-ACC SHIFTED ordering, every shift mode, signed arithmetic, PACK/UNPACK formats, wrapped additions and addresses, odd-step memory access, read/write aliasing and effect accumulation. Explicit unsigned wrapping also removes signed-overflow undefined behavior in the reference and predecoded arithmetic.

Build-time generation emits straight-line C compiled by the existing Vita compiler. Runtime generation emits ARMv7 instructions with AAPCS register preservation and synchronized code publication. Full program contents are checked after hash matches. MPRO writes invalidate that instance; reference processing remains active until Start selects a replacement. Cache eviction, translation failure and unavailable VM preserve fallback behavior.

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

The actual platform bridge also passed emulated VM lifecycle/failure/cleanup tests; corpus parsing and deduplication tests passed. Nonempty synthetic-corpus Vita compilation and packaging passed. Synthetic programs are not captured game programs. Actual captured 52/84-step equivalence, hardware sound, VM execution and performance remain to be validated.

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
3. Use multiple steady-state windows with matching program hash, 84 steps, 256 frames and profiling sample count. Target median DSP time <=3,000 us against the current 4,016 us baseline. Compare tails as well as median. This target is not yet demonstrated.
4. Check sound and responsiveness across transitions, movies, flight and battle. Measure whole-quantum time separately against 5,804 us, and compare queue depletion counter deltas; faster DSP alone does not guarantee the audio budget.

Relevant records: LagiDSPVM (each API result), LagiDSPCapture (saved/deduplicated/error), LagiDSPProgram (hash, steps, backend, translateUs, codeBytes, sizeKind, hits, misses, reason), AzelAudioDSP and AzelAudioPerf (existing DSP/whole-quantum timing plus identity/backend). Numeric timing backend values are 0=reference, 1=predecoded, 2=aot, 3=arm; these differ from configuration enum values. Preserve both log and captures for each test session. Runtime compilation time is logged separately from per-sample DSP timing.

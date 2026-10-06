# Ruins DSP invariant metadata pass

Base: `597fb08` on `feature/native-vita-audio`.
Branch: `feature/ruins-dsp-predecode`.

Azel decides. Lagi services. Neptune renders.

## Changes and invariants

The Lagi-owned DSP replacement already predecodes MPRO and resolves live input,
COEF and MADRS register pointers at Start(). Odd-step MRD/MWT selection, NXADR,
NOFL guard, PACK CLZ and the 65,536-entry UNPACK table were already present.
This pass retains those mechanisms and formats.

- Predecode a SHIFT_USED bit from effective TWT, odd-step MWT and EWT. Skip
  saturating SHIFTED when no side effect consumes it. Whenever used, SHIFTED
  still derives from the previous ACC before computing the next ACC.
- Use predecoded TABLE to select sample-local address offset/mask pairs instead
  of branching twice per memory instruction. Ring uses DEC and RBL-1; table
  uses zero and 0xffff. NXADR is added before masking, RBP after masking, with
  the same UINT32 wrapping. MADRS is dereferenced at the instruction, and COEF
  is still dereferenced for every coefficient multiply. Neither value is cached.

The generic interpreter and guarded fallback remain unchanged. The worker's
256-frame quantum, profiling cadence, yields and render/GXM code are unchanged.
No extern/Azel files are edited.

## Verification

`tests/scspdsp_equivalence.c` compares the fast and generic paths for 1,024
sample executions across 32 randomized 84-step programs. It compares all DSP
state and all 128K words of RAM after each sample; mutates live COEF, MADRS,
DEC, RBP, RBL and MIXS between samples; exercises input/IWT aliasing, odd/even
memory controls, table/ring/NXADR, saturation, and every excluded fallback
feature. Synthetic programs test semantics; they are not the actual Ruins bank.

Run with Python packages `unicorn` and `pyelftools` installed, and VITASDK set:

```powershell
python tools/run_scspdsp_equivalence.py
$env:PSP2CGC = 'C:\Dev\sdk\host_tools\bin\psp2cgc.exe'
$env:VITASDK = 'C:\Dev\VitaSDK'
$env:PATH = "$env:VITASDK\bin;$env:PATH"
cmake --build build --parallel 32
```

The differential runner compiles the actual Lagi C implementation with the
Vita ARM compiler and runs it in an ARM emulator. Only timing and libc helpers
are stubbed; the DSP struct is extracted from the existing AOSDK header.

## Hardware comparison

Install `build/Lagi.vpk` using the usual process. Use the same Vita clock,
save/start route and scene as checkpoint 597fb08. Follow normal New Game/
module transitions into Ruins, and collect several steady-state log windows
while stationary, moving and changing audio sequences. Also listen during
movies/title/transitions and check movement/input responsiveness.

Match `[AzelAudioDSP]` with `steps=84 fastPath=1`, `unpack=23 pack=10`,
`MRD=23 MWT=10 YCOEF=67`, and the same sequence/program controls as baseline.
Keep `ADRL/FRCL/YRL`, `shiftSat/shiftWrap` and `noflR/noflW` for identity checks.
Compare `[AzelAudioPerf]` records with `frames=256 dspSteps=84`, matching `seq`
and `samples`. `dsp=...us` is a sampled estimate scaled to the 256-frame chunk,
not a direct whole-quantum timer. Compare median and upper-tail DSP time over
multiple records against the approximately 4,300 us baseline. Track `total`,
`budget` (~5,804 us), `m68k`, `scsp`, `slots`, `queued`, `emptyChunks` and
`shortWrites`. Use counter deltas rather than absolute accumulated totals.

Accept only with unchanged sound/effects and responsiveness and repeatable
DSP improvement. A successful build or emulator test does not prove a hardware
speedup; address selection loads and extra control branching need measurement.
If timing regresses, retain this isolated branch for comparison and return to
597fb08. Do not attribute changing slot/68K load to the DSP optimization.

Verified 2026-10-06: ARM differential suite passed all 1,024 comparisons and fallback guards. The requested Vita build completed successfully and produced Lagi.vpk. One warning came from generated field_fieldRadar.cpp (array comparison); the DSP source compiled without warnings. On Vita hardware, the executable-memory probe returned 42 and runtime ARM exclusively executed the captured programs. The 84-step DSP median was 2,268 us (P95 2,392 us) across 42 records, versus a corrected predecoded median of 5,264 us. Whole-quantum median was 5,297 us, below the 5,804 us budget, with zero short writes. The tester approved the ARM audio path: it has almost no Disc-entry crackle and is audibly better than AOT. The quiet 2-4 second delayed effect on menu and LCS sounds is accepted with this path. Runtime log: ux0:data/lagi/lagi.log.

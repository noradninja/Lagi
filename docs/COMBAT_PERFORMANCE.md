# Combat performance investigation

Keep the fullres screen-space dither renderer and native Azel behavior intact.
This checkpoint adds measurements, not a claimed FPS improvement. The original
22 ms target / 33 ms deadline still requires matched hardware evidence.

## Baseline evidence

User-provided `lagi.log`, host modification time 2026-10-18 05:40,
2,718,738 bytes, SHA256
`DB1FDD41F4631FD29DABF2986649B50200E70B836FC404AB410B1F07EB00D268`.
The binary commit was not recorded in this log. Do not call it a matched
before/after capture against a newly built commit. The user identifies the
first two encounters as Brood Queens with more than two enemies onscreen.

Only `[BattleStartup] enter` through `[BattleAudio] restore` spans were used;
renderer mode 7 is not battle identity. Spans can include intro/results.
The first presentation window after each entry is excluded because it can
contain pre-entry frames. Remaining windows are 120 presentation intervals.

| Subbattle | User context | Scene samples | Renderer median | CPU-prep median | Finish-wait median | Build median | Window FPS range | >2-vblank intervals |
|---|---|---:|---:|---:|---:|---:|---:|---:|
| 8 | Brood Queen | 43 | 29.911 ms | 10.758 ms | 16.816 ms | 5.285 ms | 20.035–29.970 | 1014/2400 |
| 9 | Brood Queen | 49 | 29.971 ms | 10.909 ms | 16.775 ms | 5.204 ms | 20.668–29.970 | 1387/2760 |
| 4 | Not identified | 53 | 28.859 ms | 10.834 ms | 17.008 ms | 5.365 ms | 24.218–29.970 | 282/3000 |

A fourth entry has no scene samples/return marker before EOF. It is not proof
of either a completed encounter or a crash cause. Audio restoration alone is
not proof that all results, rewards and gameplay return are correct.

Renderer heartbeat medians are sampled, not complete-frame averages. Outlier
lines are intentionally biased and are not used as average costs. The 50 ms
presentation intervals explain the apparent 20 FPS step when the 33.3 ms slot
is missed; this is not proof of a 50 ms GPU workload or a broken game clock.

`cpuprep` excludes submission but includes build/lighting/composition portions;
do not add its components again. `gxmfinish` is CPU wait at completion, not an
independent GPU timestamp. Producer tasks and renderer can overlap. Old
`tasks=0` values were uninstrumented, not measured zero logic time. Likewise,
`upload` includes geometry preparation/copies, not just texture uploads.

## Added measurements

- `tasks`: native `runTasks()` elapsed time including Update and Draw callbacks.
  Excludes subsequent producer-slot waiting and frame publication. This is a
  whole task-tree sample, not per-actor attribution or pure simulation time.
- `positionSync`: steady-state dynamic vertex copy-loop time, a subset of
  build/upload. Zero on frames using full resource preparation instead.
- `positionSyncVerts`: vertices covered by that loop, not actor count.
- `submissions`: published model submission count, also not actor count.

Position-sync metrics reset at each render start, including UI/transition
frames that skip scene building. Existing task-profile publication uses the
existing producer snapshot boundary. There are no per-task clock reads or new
per-frame log lines; extra fields use the existing ScenePerf heartbeat.

The d9116fb measurement baseline's steady-state loop copies positions into base,
lighting, texture and Gouraud buffers. Full mode generally draws subdivision
buffers but also has fallback consumers; removing active copies without
auditing validity/mode-switch paths would be unsafe. Neither the current data
nor this instrumentation proves actor-count causality.

## First candidate: remove one unused duplicate write

A complete read/write audit of `g_vdp1LightingVertices` found allocation,
readiness checks, initialisation/animation writes and release, but no stream
binding or content reader. The live steady-state copy to that buffer has been
removed. The buffer allocation and all diagnostic/initialisation contracts are
retained. The base, texture and Gouraud copies and subdivision source/geometry
are unchanged in every mode, avoiding new stale-buffer or mode-switch rules.
This saves one DebugColorVertex write per copied vertex (16 bytes with the
current layout), not a demonstrated number of milliseconds or FPS.

`live_position_sync_test.ps1` executes the actual loop with no legacy-lighting
buffer declaration. It checks all active coordinates, base colour, untouched
UV/Gouraud attributes, retained static prefix, complete updates on rebuild and
empty-range counter safety. A source audit fence fails if the complete draw
implementation gains any reference beyond the existing readiness check, so a
future consumer forces a re-audit. This fence is not an automatic whole-program
alias proof. Task-profile checks still pass with the three active copies.

Compare the d9116fb measurement baseline against this candidate on the same
Brood Queen route. Finish a baseline capture before switching if already testing
it. The old 05:40 log lacks positionSync and measured task timings, so it cannot
establish a measured benefit of this candidate. No resolution, shader, dither,
lighting, draw list, simulation or audio work is reduced by this change.

## Reproduce

### Build-settings audit (no flags changed)

Checked both the current local `build-battle-dev/build.ninja` and the user's
`E:/dev/Lagi/build-battle-vita/build.ninja`: Neptune explicitly uses
`-O2 -fno-fast-math -ffp-contract=off`, while the runtime, task host, render
bridge, generated 3dEngine, animation, battle engine, Brood Queen and urchin
objects have no `-O` flag (compiler default). Neither set lists `-DNDEBUG`.
This is a CPU-cost lead, not proof that unoptimized task execution is the
dominant cause of the measured dips, nor a match to the earlier log's binary.
Await measured task timings before selecting an optimization experiment.

```powershell
.\tools\audit_vita_perf_build.ps1 -Build build-battle-vita |
    Format-Table Source, Found, Optimization, NDebug, ExplicitFastMath
```

The audit is read-only and inspects explicit per-object Ninja FLAGS/DEFINES;
it does not expand arbitrary Ninja variables, infer every compiler option
group (such as Ofast), or prove runtime assert behavior. Missing objects are
reported as missing, not assumed to share another object's settings. The
fixture checks per-source attribution, last -O/explicit fast-math precedence
and NDEBUG detection. No broad Release/global-optimization switch has been
made. A future experiment must isolate its source scope, retain assertions,
avoid fast-math/reassociation, check native fixed-point overflow/aliasing
semantics and compare the same hardware encounter before claiming a gain.

### October 18 follow-up log

The supplied 1,598,374-byte log (SHA256
`3261A93160D70BD7E8A02CC1D735CDEA771EE72579C1C865C89F1C9BB1A87AE4`)
contains one explicit overlay-0/sub-8 encounter with audio restoration.
Its sampled median task time is 9.053 ms, render 29.282 ms, CPU preparation
10.284 ms, vertex synchronization 1.674 ms, and finish wait 17.335 ms.
After excluding the first crossing window, 537 of 1800 presentation intervals
exceed two vblanks, across 15 windows (22.907–29.970 FPS).
Timings overlap; finish wait is not an isolated GPU timer. The log does not
identify the executable SHA and is not a matched before/after capture, so it
does not establish the benefit of the removed unused lighting-vertex copy.
The new waterfall changes are correctness work, not a combat performance claim.

### Hardware capture commands

## Opt-in render-bridge compiler experiment

### Three-encounter hardware follow-up

Log SHA256 `F4CAA46A72D0EB8DCDE107EDAFAFB49CC28E8F1DC667BAA83A4321E70B968B9B`
has optimizedBridge=1 and successful audio-return markers for all three spans.
The user identifies sub 8/9 as Queen fights and sub 4 as Magnata/Gelata urchins.
After the analyzer excludes each first crossing window:

| Encounter | Render median | Task median | Lowest window FPS | >2-vblank intervals |
|---|---:|---:|---:|---:|
| Queen 8 | 29.313 ms | 6.920 ms | 27.349 | 109/1920 |
| Urchins 4 | 26.954 ms | 6.349 ms | 29.845 | 1/2520 |
| Queen 9 | 29.472 ms | 6.947 ms | 27.453 | 165/2640 |

Frame pacing is better than the earlier captures, but these are not controlled
A/B runs or equal enemy-lifetime/effect workloads. Renderer medians remain near
29.4 ms for Queens; do not attribute the pacing difference to one optimization
or treat the full performance goal as achieved. UI LUT freshness work below is
correctness work and is not included in this capture.

`LAGI_OPTIMIZE_RENDER_BRIDGE` defaults OFF. ON changes only
`src/integration/lagi_render_bridge.cpp` to `-O2 -fwrapv
-fno-strict-aliasing -fno-fast-math -ffp-contract=off`. No task, native model,
animation, shader, resolution or draw-list logic changes. Assertions are not
disabled by this option. Existing resource-inventory logs include
`optimizedBridge=0/1` for attribution. This is a candidate, not a proven gain:
the task timer contains bridge calls but does not isolate their contribution.

For a matched A/B, retain the same commit, build type and encounter route:

```powershell
cd E:\dev\Lagi
git switch feature/battle-mode-bringup
git pull --ff-only
cmake -S . -B build-battle-vita -DLAGI_OPTIMIZE_RENDER_BRIDGE=OFF
cmake --build build-battle-vita -j 32
# Capture baseline log/video; preserve them before installing the candidate.
cmake -S . -B build-battle-vita -DLAGI_OPTIMIZE_RENDER_BRIDGE=ON
cmake --build build-battle-vita -j 32
```

Existing Vita toolchain/generator/fullres/dither/battle options are retained in
that build cache. Use a multiple-enemy Brood Queen fight, including attacks,
gauge charge, results and return. Compare `tasks`, presentation windows and
image/behavior, not just sampled renderer medians. The build-flag check is
`tests/render_bridge_perf_build_test.ps1 -Build <folder> -ExpectedOn $true`
(or `$false`). Keep OFF if gains are absent or fidelity regresses. This does
not resolve the separately documented water-layer or cinematic gaps.

```powershell
cd E:\dev\Lagi
git switch feature/battle-mode-bringup
git pull --ff-only
cmake --build build-battle-vita -j 32
```

Keep the same resolution, render/debug mode and dither settings. Capture one
Brood Queen fight with multiple enemies alive, then after enemies are removed,
including attacks/effects. Retain commit, video, fresh log and matching ELF for
any crash. Compare both frame pacing and image correctness on the same route;
do not infer improvement from a lighter scene or a different debug mode.

```powershell
.\tools\analyze_combat_perf.ps1 -Path 'D:\Users\Noradninja\Downloads\lagi.log' -AsJson
```

The read-only analyzer excludes outside/UI-only samples and initial crossing
windows, retains incomplete encounter data, and reports absent timing fields
as null. Tests cover those boundaries. The task profiler fixture executes the
actual timing/publication block with a mocked clock; source checks retain all
active payload copies and frame reset. These checks do not establish low overhead
on hardware or an optimization win.

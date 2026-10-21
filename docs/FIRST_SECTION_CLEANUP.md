# First-section cleanup goals

Work branch: feature/first-section-cleanup, from accepted main merge 92470e9.
One numbered goal is active at a time. Replace it with the next only after
the required evidence is accepted; do not call source/build tests Vita proof.
Independent corrections remain atomic commits.

1. **Active: shared combat performance and encounter logic.** Target ideal
   25 ms/frame, maximum 33 ms/frame; investigate the first slow Gelata encounter
   without enemy-specific shortcuts. Audit RNG/seeding, accumulation, story
   gates, selection and cadence without arbitrary rate increases.
2. Combat bonus rewards and earned Berserk progression: eligibility, native
   calculation, inventory/unlock state, display and save persistence.
3. Saved-game fade/black hold; Valley-to-Excavation fade; keep-awake ticking
   during in-engine cinematics as well as Cinepak, without changing simulation.
4. All tunnel enter/leave audio, immediate switch sound fade, native switch
   color/gloss behavior.
5. Excavation cinematic/town floor corruption: shared source/native black
   behavior, not a scene-specific mask.
6. Second field-map acquisition/entry failure, visited-area marking, selection,
   exit and save/reload correctness.
7. Native battle tutorials and first-section item/Berserk coverage, including
   Blast Chip and Energy Prism trails, lighting, camera, timing and cleanup.

## Goal 1 baseline (complete-run H:/data/lagi/lagi.log)

The first logged encounter is BTL_A3 sub 5. The user's identification is
Gelata Urchins. Its 14 complete presentation windows contain 1680 intervals,
826 longer than two VBlanks; window rates span 19.980-29.969 FPS. Sampled
renderer median is 31.063 ms, CPU preparation 14.406 ms, dynamic object append
5.207 ms, GPU finish wait 14.955 ms. Geometry median is 1616 polygons, 9696
vertices, 204.5 submissions. Texture-cache hits dominate; material resolution
is not the observed steady append cost. Later sub 8/9 spans have medians near
25.9 ms, 992 polygons and 138 submissions, with no missed two-VBlank intervals
in their complete windows. These are different workloads, not a matched A/B.
Profiler fields overlap; do not sum tasks/build/sync/cpuprep as independent time.

The audio profiler also reports several over-budget quanta around this fight.
This is not yet isolated as a rendering cause. Backend labels need care:
program-selection logs report ARM compilation/hits, while some sampled SCSP
quanta report backend 0. Do not infer the selected execution path from one
label or change DSP/audio semantics without inspecting its dispatch boundary.

## Encounter audit so far

The generated runtime compiles Azel dummy.cpp's RNG and titleScreen.cpp's
seeding path, not a platform rand() replacement. Title initialization seeds
zero and advances the generator as the native title task runs; user timing
affects subsequent state. Encounter probability and weighted formation
selection both call randomNumber()/performModulo2(100,...).

Existing encounter_trace_test verifies generated area/probability/RNG/counter
code is unchanged after observer calls are removed. This demonstrates faithful
use of the reconstruction, not independent verification of every original
Saturn instruction. Original Disc 1 COMMON data at LBA 182 confirms:

- selected field-2 selector 00208744: reset 150 updates, initial 120, max 20
  failed attempts;
- row 00207660: chance 8, weighted formation IDs 5/7 at 50/50;
- row 00207680: chance 8, weighted formation IDs 2/4 at 10/90.

The log contains failed 8% rolls followed by successful rolls, including a
later hit after 13 failures. It also records disabled-status gates and areas
with no row. Absence of a named enemy in one run does not establish an RNG
bug. At 30 updates/second, a 150-update reset is five moving seconds; stopping,
disabled gates and area changes affect the real elapsed opportunity. Native
cadence, original instruction equivalence and formation-name mapping remain
audit work; no encounter tuning is applied.

encounter_rng_test executes the generated RNG for one million output/state
comparisons against an independent arithmetic form, including zero-low-word
reseed and unsigned modulo behavior. It also verifies the original COMMON
metadata above. This does not assert that a full run must encounter any named
formation. The test now additionally checks the complete original MAIN RNG
instruction stream at 060361F4..06036218, both seed-address literals at
060361F0/0603621C, the 2A6D365B fallback and the initializer at 060361E8.
The original multiplies by 41, folds product halves, swaps the return halves
and stores the next state in the return delay slot, matching the reconstructed
routine exercised by the host fixture. This is an instruction/data audit,
not a whole-system SH2 execution comparison. The existing encounter
observer/evaluation-equivalence test also passes.

Further original MAIN checks cover 0600CB3E..0600CB8A: load countdown,
decrement once, signed positive test and delay-slot store; unsigned chance
versus modulo-100 roll; failed-attempt increment/maximum compare; signed
16-bit reset reload. RNG and modulo literals are 060361F4 and 06035550.
Thus there is no extra wall-clock encounter timer to accelerate independently.
The Vita task host calls Update once per unpaused task traversal; rendering
slowdown can reduce checks per real second. Do not compensate by adding extra
RNG calls or changing native reset values. A full native task-scheduler timing
comparison is not claimed by these bounded checks.

The disc-backed production COMMON remapper/BTL_A3 entry parser now reports
all ten reachable A3 subbattle mappings. IDs 0/1/2 call Baldor formation;
3 and 5 call Urchin configuration 3; 4 calls Urchin configuration 4;
6/7 call Patergo configurations 6/7; 8/9 call Baldor Queen formation.
Consequently the first tested row's 50/50 IDs 5/7 are Urchins/Patergo, and the
later row's 10/90 IDs 2/4 are Baldor/mixed Urchins. The supplied run's absence
of Patergo/Baldor is compatible with those weights, not evidence that their
native dispatch is absent. No enemy appearance guarantee or rate tuning is
introduced. Original instruction/data checks and reconstructed task/gate code
are separate evidence; full scheduler parity remains outside these fixtures.

## First shared optimization

Dynamic model append previously resized default-initialized polygon/light
arrays and then overwrote their payloads. It now copies descriptors directly
into final storage and constructs each submission's shared light values
directly. Gouraud appended slots are already zeroed by resize, so the second
zero pass is removed. Geometry transforms, normals, palettes, light math,
blend order and actor selection are unchanged. No model/encounter ID is used.

live_model_append_test extracts both the production function and accepted
92470e9 baseline. It compares resulting geometry, lighting vertices, material
indices, polygon lighting/descriptors, zero shade payloads and ordered ranges
across transformed, billboard, native-light, projection-offset, partial mapping,
repeated-append and noncanonical-layout cases. Host equivalence is not timing.
Both full-resolution battle and menu-profile Vita VPK builds pass.

Next hardware evidence: fresh combat log covering any complex encounter,
preferably including an Urchin formation when naturally encountered. Exact
run matches are not required. Compare workload-aware append/build/render and
presentation-window distributions; confirm effects/lighting remain intact.
Goal 1 remains open, and goal 2 must not begin merely because this patch builds.

# Lagi development status

**Updated 21 November 2026**
**Working combat baseline:** `diagnostics/garil-cliff-current` at `977809e`, with documentation changes following that baseline.

This is the current project overview. The older, dated engineering reports remain useful for source addresses and test evidence, but they are not competing reports on the present build.

## Where the game stands

The native boot and game sequence reaches the title, name entry, Ruins, flight, Excavation and Valley battles. We have since brought up world-map and camp routes and the first Garil Desert encounters. Not all story events, formations, visual effects or returns are complete.

The main game remains Azel-driven. Lagi supplies resource loading, native Vita services, graphics and audio integration, and synchronization; Neptune renders the captured game presentation. There is no substitute battle engine or separate Vita simulation of the original gameplay.

## Implemented systems

| System | Current coverage |
| --- | --- |
| Startup and disc access | Original movies, title, name entry, accepted four-disc selector, CUE/BIN mounting and disc-backed assets |
| Field and towns | Native tasks, scripts, flight, collision, interactions, cinematics, radar, menus and field rendering |
| Battle | Native A3/A5 bring-up, targeting, weapons, enemy lifecycles, results/rewards and shared progression |
| Graphics | VDP1 geometry, sprites, effects and Gouraud; VDP2 normal/rotation layers, priority, color operations, windows and fades |
| Audio/video | Native audio and SCSP DSP runtime ARM backend; Sega FILM/Cinepak playback |
| Persistence | Vita save/load integration and preserved progression fields; see the save-format notes |

These features have different validation histories. A linked task or successful damage calculation does not establish that its complete visual effect or story transition is working.

## Current Garil/Valley work

The restored combat baseline is commit `977809e`. It brings back the exact tracked tree of `60d7b61` after experimental WEAK marker depth and tracing changes regressed Valley combat. Those reverted experiments remain in Git history, but are not part of the active tree.

Recent Garil development includes the original horizon sprites, native Nanyd targeting, enemy life-meter retirement, queen-loss Scatter handoff, the original Scatter text path, escape offsets/movement and authored quadrant eligibility for WEAK cursors. The user confirms Scatter and the repaired second battle work (21 November, second battle at a70d109). Its missing Gash horn warning is now bound to the shared battle-dialogue service, and the story-worm actor/controller/camera wiring has been restored; both await the next Vita run. The user confirms the main exit is gated by the unimplemented Lathum fight; its current pre-Lathum unavailability is expected. Bounded script/LCS observations remain for the eventual post-Lathum test. Garil shows repeated missed 30 Hz deadlines (about 20.6 fps in the slowest steady log window), and graphical glitches await video. Remaining checks include those scenes, post-Lathum main-exit selection, full results/field return, repeated encounters and visual/performance parity. Camp/Gash story continuity is another original-script validation boundary.

The story and environmental worms now share the native segment sand and X/Z-following particle producers, including safe anchor lifetime through actor/pool teardown; their appearance still awaits hardware confirmation. The shared field-particle pool now releases finished particles' task-owned payloads as Saturn does. This fixes retention until scene teardown and growing allocation-list traversal; its effect on Garil frame pacing is not yet measured.

The shared type-1 field camera now calculates roll from authored keyframe endpoints and writes it to the roll axis, matching the original instructions. Host/native-data checks and the Vita package pass; cinematic visuals still need hardware confirmation.

The full-screen colored rectangles in the latest video correlate with a source-proven wind sprite table error: the table address was decoded as sprite data. Wind emitters now dereference the authored pointer tables, preserve their slot texture base/animation phase and restore native sand gravity. Host/native-data tests and the Vita package pass; visual confirmation remains pending.

A source implementation, a host fixture and a hardware-verified scene are different things. We record that difference because these paths have repeatedly compiled and still shown issues on Vita.

[Garil shared parity](GARIL_SHARED_PARITY.md) keeps the original addresses, tests and historical commits. [Battle effects recovery](BATTLE_EFFECTS_RECOVERY.md) describes missing native effect producers.

## Berserks

The original Berserk dispatcher and several attack tasks are present, but a number of producers still omit models, trail update/draw routines, homing projectiles, particle emitters, damage loops or completion handling. We're restoring their shared mechanisms instead of building one-off Vita effects. The verified Disc 1 proficiency table, item IDs and skill names are in [Berserks: Disc 1](BERSERKS_DISC1.md).

## Configuration and performance

The current `CMakeLists.txt` defaults ON for `LAGI_FULLRES`, stochastic filtering, dither lookup, native A3 battle execution, WORLD bring-up, VDP2 sprite composition, RBG page/row optimizations, and Neptune/render-bridge optimization. GPU-pass timing, battle link audit and field-motion tracing default OFF.

Older CMake build directories may cache very different settings. Always compare matching configurations, scenes and effects. **30 Hz is the target, not a guarantee** for every encounter or menu.

For current commands use [Building Lagi](BUILDING.md). For underlying architecture see [Architecture](ARCHITECTURE.md) and [Neptune](NEPTUNE_RENDERER.md).

## Validation and releases

A source trace establishes the original instruction or data behavior; a host test checks a specific implementation; a Vita run validates the actual experience and performance. We distinguish those claims in the technical notes.

Current development is well ahead of the public `v0.3.0-alpha` historical milestone, but this is not a claim of full Disc 1 completion or a new release.

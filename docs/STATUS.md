# Lagi development status

**Updated 20 November 2026**  
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

Recent Garil development includes the original horizon sprites, native Nanyd targeting, enemy life-meter retirement, queen-loss Scatter handoff, the original Scatter text path, escape offsets/movement and authored quadrant eligibility for WEAK cursors. The remaining checks are the appearance and depth of WEAK markers, Scatter text and survivors' flight, results/field return, repeated encounters, and later worm/story sequences. Camp/Gash story continuity is another original-script validation boundary.

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

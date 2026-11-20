# Documentation guide

Lagi has moved through several major renderer, runtime and battle milestones. This folder contains both **current reference pages** and **dated engineering records**. The older records are still useful, but a build option or incomplete-feature note from October is not necessarily true of the current main branch.

## Current references

| Document | Purpose |
| --- | --- |
| [Project README](../README.md) | Project overview and current scope |
| [Development status](STATUS.md) | Current playable route, Valley/Garil baseline and open hardware checks |
| [Building Lagi](BUILDING.md) | Current CMake defaults, Windows paths and build commands |
| [Architecture](ARCHITECTURE.md) | Ownership between Azel, Lagi and Neptune |
| [Neptune renderer](NEPTUNE_RENDERER.md) | Shared rendering and presentation systems |
| [Berserks: Disc 1](BERSERKS_DISC1.md) | Proficiency tables, item IDs, native effect dependencies |
| [Battle effects recovery](BATTLE_EFFECTS_RECOVERY.md) | Missing effect producers and shared services |
| [Garil shared parity](GARIL_SHARED_PARITY.md) | Original A5, Scatter, escape and WEAK reticule work |
| [SCSP DSP translator](SCSP_DSP_TRANSLATOR.md) | Native DSP implementation and fallback modes |
| [Disc selection](DISC_SELECTION_FRONTEND.md) | Four-disc selector and mounted game resources |

## Engineering history

The detailed [battle bring-up](BATTLE_MODE_BRINGUP.md), [battle acceptance](BATTLE_ACCEPTANCE_2026_10_20.md), [graphics handoff](BATTLE_GRAPHICS_HANDOFF.md), [combat performance](COMBAT_PERFORMANCE.md), [flight bring-up](FLIGHT_MODE_BRINGUP.md), [world-map handoff](WORLD_MAP_HANDOFF.md), [work-machine notes](WORLD_MAP_WORK_BUILD.md) and [first-section cleanup](FIRST_SECTION_CLEANUP.md) preserve source addresses, tests and earlier decisions.

Other files in this directory document individual audio, cinematics, menus, lighting, rendering and save-system changes. Their dated measurements and branch names are part of that history. The current [status](STATUS.md) and [build guide](BUILDING.md) are the sources for today's working baseline.

Our evidence has three levels: original disc/source behavior, host/build verification, and actual Vita hardware results. They are not interchangeable.

**Azel decides. Lagi services. Neptune renders.**

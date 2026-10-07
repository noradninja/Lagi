# Flight Mode Bring-Up

Lagi's first flight milestone begins immediately after the Ruins elevator movie sequence. Azel advances game status 5 to status `0x50`, which maps to game mode 3 / field index 1 and loads `FLD_A3.PRG` ("above excavation").

The implementation follows the same ownership rule used for the Ruins runtime:

```text
Azel decides.
Lagi services.
Neptune renders.
```

Flight behavior, field scripts, dragon movement, camera state, visibility, animation, encounters, VDP1/VDP2 state, and progression remain Azel-owned. Lagi restores the Vita-facing services and presentation paths required to let that runtime execute natively.

## Runtime path

```text
Ruins elevator sequence
        ↓
EVT004_1.CPK / EVT004_2.CPK
        ↓
Azel evaluates game status 5
        ↓
setNextGameStatus(0x50)
        ↓
game status 0x50
        ↓
game mode 3 / field index 1
        ↓
loadField(...)
        ↓
FLD_A3.PRG
        ↓
overlayStart_FLD_A3(...)
        ↓
initField(...)
        ↓
field task graph
        ↓
dragon / camera / visibility / scripts / encounters
        ↓
activateDragonFlight()
```

## Milestone ladder

```text
A — FLD_A3 loads without crash
B — field task graph runs
C — first 3D geometry visible
D — correct field camera
E — dragon visible
F — dragon animation
G — player-controlled flight
H — correct field visibility / LOD
I — VDP2 / background presentation
J — radar / LCS / field UI
K — field effects and native ray/laser presentation
L — progression to the next Azel game state
```

## Phase 1 — Native FLD_A3 entry

Restore `overlayStart_FLD_A3` to the generated Vita field dispatcher and instrument the transition without changing rendering behavior.

The hardware log should establish:

```text
game status 5 completes
→ request 0x50
→ game mode 3 / field 1
→ FLD_A3.PRG selected
→ overlayStart_FLD_A3
→ initField
→ common field resources
→ field file list
→ field script task
→ dragon task
→ field overlay tasks
→ camera task
→ encounter task
→ sound-bank request
```

Acceptance gate: Azel reaches a stable FLD_A3 field task graph, even if the screen remains black. No field-specific Neptune rendering is added in this phase.

## Phase 2 — Mode 3 presentation ownership

Extend Lagi's generic scene capability check from town-only mode 1 to field mode 3, and add a field presentation adapter to `lagi_scene_bridge`.

The adapter is a presentation boundary, not a second field runtime. Azel remains authoritative.

Acceptance gate: mode 3 participates in the existing producer/consumer presentation boundary without regressing towns or movies.

## Phase 3 — Field presentation adapter

Expose renderer-facing state already produced by Azel:

- field camera and projection/FOV
- static field geometry
- visibility results
- dynamic field objects
- dragon world pose and animated hierarchy
- rider hierarchy
- active lighting
- VDP1 command stream
- VDP2 state

Reuse the existing generic Azel render bridge and Neptune material/geometry paths.

Acceptance gate: recognizable Excavation geometry appears, even if transforms are not yet correct.

## Phase 4 — Transform semantics

Determine whether each field submission is world-space or already view-relative, then use scoped bridge metadata to prevent double-camera transforms.

Validate terrain, dynamic objects, dragon, rider, and effects separately.

Acceptance gate: stable world geometry, correct dragon/world relationship, and no mirroring or double-camera errors.

## Phase 5 — Dragon presentation and control

Consume Azel's existing field dragon task. Do not implement Vita-specific flight logic.

```text
Vita controls
   ↓
Saturn input state
   ↓
Azel dragonFlightUpdate()
   ↓
Azel position / orientation / animation
   ↓
Neptune presentation
```

Acceptance gate: visible animated dragon and rider with working flight controls.

## Phase 6 — Field camera

Publish Azel's native field camera, including the configured follow mode and field FOV.

Acceptance gate: framing and flight response agree with Saturn reference behavior.

## Phase 7 — VDP2 / background

Bring field VDP2 state online incrementally. Log unsupported state first; do not block the first 3D milestone on perfect background composition.

Track BGON, NBG/RBG use, scroll, line scroll, CRAM, priority, color calculation, windows, and back-screen state.

## Phase 8 — Field UI

Reconnect the field radar, LCS/lock-on, HUD, text, and other VDP1/VDP2 UI through Azel's own state and command streams.

## Phase 9 — Field effects

Restore field-specific effects that were intentionally suppressed during the boot-to-Ruins milestone. In particular, A3 ray/laser drawing currently bypasses upstream desktop BGFX/GLM rendering and must be translated to Neptune rather than re-enabling the desktop path.

## Phase 10 — Saturn comparison and progression

Compare the same flight sequence against Saturn for spawn position, orientation, camera, movement response, animation, visibility/LOD, geometry, lighting, radar/LCS, audio, and script progression.

The milestone completes when Azel naturally advances from the supported FLD_A3 sequence into its next game state.

## Deferred audio optimization

Audio performance work is intentionally separate from flight bring-up. The current deferred branch is `feature/audio-affinity-profiling` / PR #11. It preserves the existing CPU2 default and adds hardware A/B affinity testing for CPU 0, CPU 1, CPU 2, and all user cores. A dedicated ARM DSP worker is considered only after that profiling establishes a worthwhile synchronization target.

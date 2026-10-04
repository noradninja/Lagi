# Lagi Runtime Architecture

Lagi is a native PlayStation Vita host for the reconstructed *Panzer Dragoon Saga* runtime in `extern/Azel`.

## Core ownership rule

The architecture is:

```text
Azel decides.
Lagi services.
Neptune renders.
```

Azel owns game/runtime behavior:

- startup and reset state;
- game modes and module transitions;
- title/menu state;
- New Game flow;
- movies and post-movie status changes;
- field/town/battle task graphs;
- scripts;
- camera behavior;
- collision;
- animation;
- visibility and object lifetime;
- VDP1 command generation;
- VDP2 memory/register state;
- fades and transition timing.

Lagi owns the Vita boundary around that runtime:

- physical input translation;
- disc/filesystem access;
- VBlank/timing services;
- native audio;
- memory/resource adaptation;
- frame synchronization;
- renderer-facing presentation snapshots.

Neptune owns presentation of those snapshots through native SceGxm.

`extern/Azel` is treated as upstream source and is not modified for Vita integration.

When Azel depends on desktop/Saturn-facing host services, adapt those services at the Lagi boundary. Do not reproduce gameplay logic in Vita-specific code.

## Native runtime host

The normal Vita host loop is now `src/integration/lagi_runtime.cpp`:

```text
Vita input
    ↓
Lagi input bridge
    ↓
Azel input state

VBlank / fade service
    ↓
begin Azel presentation capture
    ↓
runTasks()
    ↓
capture renderer-facing state
    ↓
generic scene bridge
    ↓
generic presentation publish
    ↓
Neptune render thread
```

The old `runtime_smoke_*` host has been retired. The memory-reader smoke check and diagnostic tracing now live in dedicated diagnostic code rather than defining runtime ownership.

## 0.040 authentic boot path

The 0.040 branch no longer enters Ruins through the old direct-development loader.

The active path is:

```text
Disc 1
  ↓
azelInit()
  ↓
resetEngine()
  ↓
native Azel initial task graph
  ↓
MOVIE1.CPK
  ↓
title / New Game
  ↓
EVT000_1.CPK
  ↓
FLD_D5 name-entry sequence
  ↓
EVT002.CPK
  ↓
Azel module manager
  ↓
TWN_RUIN.PRG
  ↓
native town task graph
```

Lagi does not choose `TWN_RUIN`, construct the town task graph, or manually call the Ruins overlay as part of the normal 0.040 path.

The older direct-boot/bootstrap code remains in the tree as a development/reference path while the authentic route is brought fully online, but it is not the game-mode owner.

## Generic scene presentation bridge

Lagi should not conceptually own a town, battle, field, or other game mode.

The runtime calls a generic scene bridge:

```text
Azel active scene
    ↓
lagi::scene_bridge::sync_presentation_state()
    ↓
mode-specific adapter
    ↓
generic renderer-facing presentation state
    ↓
Neptune
```

Today, mode 1 dispatches to the existing town adapter because that is the first native 3D scene class Neptune can present. That adapter exposes renderer-facing state from Azel; it does not own the town.

As support expands, other mode adapters can join the same bridge without changing host-loop ownership.

The platform-facing renderer API is intentionally mode-agnostic:

```text
show_game_presentation()

presentation_wait_frame_slot()
presentation_publish_frame()

presentation_set_player()
presentation_set_camera()
presentation_set_vdp2_text()

presentation_fade_in()
presentation_fade_out()
```

Internal Neptune variables may still carry historical `g_town*` names. Those names are implementation debt, not the architectural contract.

## Frame ownership and synchronization

The presentation bridge uses a one-frame producer/consumer boundary between the Azel/game thread and Neptune render thread.

The critical rule is that the render slot is acquired at the **publish boundary**, not before `runTasks()`:

```text
runTasks()
    ↓
collect staging state
    ↓
sync scene presentation
    ↓
acquire presentation slot
    ↓
publish immutable frame
    ↓
Neptune owns published frame
    ↓
render thread returns slot
```

This prevents a game-mode transition during `runTasks()` from deadlocking with another presentation path such as movie playback.

Movie and front-end VDP2 presentation participate in the same renderer ownership protocol.

## VDP1 presentation

Azel emits native VDP1 state/commands. Lagi captures renderer-facing output; Neptune translates that output to native GXM.

Current hardware-proven command coverage includes:

- normal sprites;
- scaled sprites;
- polylines;
- town/world geometry submissions used by the first Ruins slice;
- Edge and task-owned object submissions;
- Edge's original mesh shadow path.

The bridge must translate Azel's commands rather than recreate lock-on, menu, cursor, animation, or gameplay state.

## VDP2 presentation

Neptune is evolving toward a Saturn VDP2 renderer rather than a collection of screen-specific renderers.

Conceptually:

```text
Azel
  ↓
native Saturn VDP2 state
  ↓
VRAM / CRAM / registers
  ↓
Neptune VDP2
  ├─ NBG
  ├─ RBG
  ├─ back/line/color state
  └─ composition/windows
  ↓
SGX
```

The current front-end path already consumes live Azel VDP2 state for:

- title NBG presentation;
- D5 NBG0 keyboard;
- NBG3 text;
- D5 RBG0 rotation-map state;
- line-window composition;
- live CRAM;
- VDP2 color offsets.

D5 is not intended to have a bespoke background renderer. It is a client configuration of generic VDP2 facilities.

The current RBG0 implementation still has accuracy work remaining, especially complete Saturn compositing/window behavior and color/fade accuracy.

## Movie pipeline

Movie ownership follows the same rule as scene ownership:

```text
Azel movie task/state machine
       ↓
Lagi movie backend services
  ├─ disc/filesystem reads
  ├─ Sega FILM demux
  ├─ Cinepak decode
  ├─ PCM/audio timing
  └─ Neptune frame presentation
```

Azel owns:

- movie selection;
- multi-part sequencing;
- skip behavior;
- fades;
- subtitle-task creation;
- post-movie status transitions.

Lagi must not hardcode scene-specific movie sequencing in the platform layer.

The current Vita path includes native SceAudio and SGX-assisted Cinepak presentation while preserving Azel sequencing ownership.

## Direct-boot compatibility

The earlier direct-Ruins path was valuable for renderer and runtime bring-up, but it is no longer the normal boot architecture.

Direct-boot compatibility code may remain useful for isolated development and regression testing. It must not become a hidden prerequisite for authentic boot.

In particular, authentic scene presentation must not require state that is initialized only by:

- `init_town_bootstrap()`;
- `init_town_runtime()`;
- manual `overlayStart_TWN_RUIN()`;
- reconstructed static-room viewer registration.

Any renderer resource or state required by the native path must come from generic platform services or the live Azel presentation stream.

## Upstream integration rule

Where Azel's desktop umbrella header cannot be used on Vita, the build may redirect that include to a Lagi platform prelude.

That prelude may provide declarations or stubs for unavailable host services, but it must not alter or replace Azel gameplay/runtime function bodies.

Generated Vita copies/adapters may replace host-facing calls where needed, while upstream task/state-machine ownership remains authoritative.

# Lagi Runtime Architecture

Lagi is a native PlayStation Vita host for the reconstructed Panzer Dragoon Saga runtime in `extern/Azel`.

## Runtime ownership rule

Gameplay and runtime behavior belongs to upstream Azel.

Files under `src/integration/lagi_*` may:

- provide Vita platform services to Azel;
- adapt input, filesystem, audio, rendering, timing, and memory interfaces;
- expose Vita/GXM data to Azel;
- schedule or enter Azel task/runtime pipelines;
- bridge Azel output into Vita-specific presentation.

Direct-boot compatibility belongs at the same boundary. It may restore platform state that the normal Saturn startup path would already have established—for example VDP2 initialization, queued VDP2 transfer servicing, or resident VDP1 UI data—but it must not replace the gameplay task or author new game state.

They must **not** independently reproduce gameplay/runtime behavior that already exists in `extern/Azel`.

In particular, do not create a second implementation of Azel NPC movement, scripts, camera behavior, collision behavior, animation state machines, task state, or scene sequencing merely to make it portable.

When Azel code depends on a desktop-only service, adapt that service at the boundary. Do not translate the gameplay function into a Lagi-owned equivalent.

## Town pipeline

The intended Ruins path is:

```text
TWN_RUIN script
       ↓
Azel script/runtime pipeline
       ↓
Azel setupNPCWalkInZDirection()
       ↓
Azel sEdgeTask::Update()
       ↓
Azel updateEdgeSub2()
       ↓
Azel stepNPCForward()
       ↓
Lagi platform/render/input adapters
```

`extern/Azel` is treated as upstream source and is not modified for Vita integration.

Where Azel's desktop umbrella header cannot be used on Vita, the build may redirect that include to a Lagi platform prelude. That adapter may provide declarations/stubs for unavailable host services, but it must not alter or replace Azel gameplay/runtime function bodies.

## Presentation bridge

The current town presentation path keeps state ownership on the Azel side:

```text
Azel tasks / scripts
       ↓
processed 3D models + VDP1 commands + VDP2 state
       ↓
Lagi frame-boundary snapshots and platform adapters
       ↓
Neptune GXM geometry, sprite, line, tile, and text batches
       ↓
2x MSAA hardware resolve
       ↓
Vita display
```

Neptune translates the VDP1 normal-sprite, scaled-sprite, and polyline commands needed by the first Ruins sequence. It also translates the live VDP2 NBG1 window map, NBG3 text map, CRAM palettes, and vertical line-scroll state used by the area banner, item messages, interaction subtitles, cinematic matte, and elevator choice.

The bridge preserves the original layer order and command data. It does not own Lock-On selection, dialog text, choice state, cursor animation, or fade sequencing.

## Direct-boot state restoration

Direct town boot bypasses portions of the original full-game startup path. The Vita adapter therefore restores only the platform state that the upstream town runtime expects:

- initializes VDP2 and resets its string state before the town overlay starts;
- services queued VDP2 register/VRAM work at the frame boundary;
- snapshots completed VDP2 state for Neptune after Azel tasks run;
- loads `MENU.CGB` at the original resident VDP1 byte address used by menu sprites.

These steps supply the startup prerequisites that the normal engine path would have established. The source assets, script flow, UI commands, and gameplay decisions still come from Azel and the original disc data.

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

# Neptune — native Vita rendering architecture and feature map

> Scope: `feature/menu-save-load-restoration`, 2026-10-12. This describes implemented paths and separates source-level support from hardware-validated visual accuracy. Neptune is a renderer, not a Saturn emulator or a replacement for Azel's task graph.

## Responsibility and frame flow

**Azel decides. Lagi transports. Neptune renders.** Azel owns game tasks, cameras, model transforms, lighting selectors, VDP1 commands, VDP2 registers and VRAM/CRAM, animation, particle behavior, priorities and fades. Lagi captures and adapts native state without duplicating game decisions. Neptune consumes a published, stable frame and translates it to **SceGxm/SGX** draw and sampling operations; VitaGL is not used.

```text
Azel task graph / VDP1 commands / VDP2 state
       |
       v
Lagi native capture + resource adapters
       |  begin_frame -> runTasks -> publish_frame
       v
Immutable presentation frame / model and UI submissions
       |
       v
Neptune render thread -> SceGxm -> selected display framebuffer
```

The producer/consumer presentation-slot boundary also serves movie and front-end presentation. Native model identity and texture generations are tracked separately; static and dynamic submissions share the rendering service. Do not introduce scene-specific texture IDs, hand-authored draw order or replacement cameras.

## VDP1 — geometry, sprites, effects

| Feature | Current Neptune service / behavior |
| --- | --- |
| Textured polygonal models | Adapts native Azel processed models and Saturn polygon records; triangulates original quads for GXM while retaining per-quad attributes. |
| Town and field geometry | Shared renderer; field and town have distinct established coordinate/winding conventions, not separate game renderers. |
| Dynamic actors | Native task-owned model submissions, including Edge, dragon and rider, with captured transforms and resource identities. |
| Normal and scaled sprites | Translates Azel VDP1 UI commands, coordinates, source addresses and color/pixel mode state. |
| Distorted quads, polylines and cursor/effects | VDP1 command-oriented rendering, including HUD/LCS and line-based effects; coverage and exact command modes should be checked per path. |
| Textures and palettes | Saturn VDP1 texture decode/cache, palette dependency and selective VRAM/CRAM invalidation; geometry-only changes need not force texture reupload. |
| Gouraud | Four-corner RGB555 values and native lighting modes are retained; GXM shaders interpolate corner/scanline color. Flat-authored faces must remain flat. |
| Directional lighting | Native per-submission vector, color and falloff captured from Azel and consumed by lit meshes; cached static lighting can refresh independently of geometry. |
| Point lights | A11D selector capture, native matrix-relative point evaluation, signed model-space vector, native colors/falloff and single-consumption path; **visual correctness still under hardware validation**. |
| Shadows and transparency | Existing mesh-shadow/stipple behavior, polygon opacity and native composition conventions. |
| Particles | Azel owns animation, orbits and lifetimes. Neptune draws native animated-quad sprites, sparkle/orb and trail Gouraud color tables, rays and other world effects. |
| Depth and ordering | Preserves producer world depth, normalized sprite depth and native sort metadata; separates background/world effects/screen overlays without hard-coded item identity. |
| Filtering | World/map/radar paths preserve intended point sampling; selected scaled UI, menus and title use screen-space dithered sampling with hardware bilinear available for comparison. |
| Diagnostics | Textured, textured-Gouraud, grayscale-lighting, polygon-color and wireframe modes are exposed by `Vdp1RenderMode`. |

### Lighting detail

The point-light implementation is **not** an invented modern inverse-square shader. It reconstructs native selector push/pop/reset and the SH7604-style distance, integer root, division and cutoff operations. The bridge captures light state at the actual `addObjectToDrawList()` boundary. `SubmissionState::nativeModelLight` is already transformed into model space; Neptune must **not** rotate it again. The field collection effect is intentionally scoped to dragon/rider on original Saturn hardware, not the environment. Town/environment lights require their own native task paths. The special light position must use the original *transform-and-add* matrix operation, unlike ordinary directional vectors.

**Validation status:** selector and math host tests, shader compilation and syntax checks do not prove on-device color or Gouraud fidelity. Recent capture found out-of-range zero vectors caused by missing position translation; the correction exists, but corrected hardware appearance remains pending. See [NATIVE_POINT_LIGHT_RESTORATION.md](NATIVE_POINT_LIGHT_RESTORATION.md).

### Particle ordering

World-space particles, rays and pickup effects must sit behind the native pickup NBG1 panel and NBG3 glyphs, while late screen overlays (including radar) remain in front. The classification uses Azel's submitted world-depth metadata rather than texture names. The animated-quad adapter `lagiDrawParticleWithColorTable` preserves Saturn-authored Gouraud tables, including frame-indexed orb colors and overlapping trail windows. See [PICKUP_COMPOSITION.md](PICKUP_COMPOSITION.md).

## VDP2 — backgrounds and composition

| Feature | Current Neptune service / behavior |
| --- | --- |
| Normal scroll backgrounds | NBG tile/map/palette interpretation, including title, menus, D5 keyboard and NBG3 text. |
| Rotation backgrounds | RBG0 maps, rotation parameters, coefficient-driven sampling and A/B-related state from Azel. |
| VRAM/CRAM | Captured Saturn memory and live palette state, dependency-aware updates and selective invalidation. |
| Windows | Native VDP2 line/window masks and layer visibility/composition. |
| Priorities | Composes VDP2 layers and VDP1 world/UI passes using native state and producer ordering metadata. |
| Color calculation and fades | VDP2 color offsets, fade color/direction and per-frame native state. |
| Line effects | Line scroll, subtitle matte/black bars and other line/window composition. |
| Front-end | Title high-resolution mode, name-entry backgrounds, keyboard, text, save/load and System menu presentation. |
| Field backgrounds | Sky, rotation backgrounds, mountain/horizon layering and scene presentation; incorrect floor/map interpretation remains under investigation. |
| Sampling and output | Source-raster VDP2 interpretation on SGX, then final sampling/scaling to the selected Vita framebuffer; point-exact game backgrounds and dithered scaled UI where selected. |

The RBG0 interpretation path resolves Saturn source pixels into a source-sized RGBA surface before GPU point scaling. This avoids repeatedly executing expensive map/coefficient interpretation at the larger output size. The current title mode uses a 720×408 framebuffer, gameplay defaults to 480×272 (with optional 960×544 configuration), and Cinepak has its own presentation mode. These are **selected framebuffer/display modes**, not a claim that everything is rendered to a full-resolution intermediate.

## Shared composition and movie integration

Neptune composes 3D geometry, depth-aware VDP1 world effects, VDP2 background layers, UI backing tiles, text, late HUD sprites and fades in the required native order. This is a generalized presentation service, not a collection of special cases for individual screens. The movie backend demuxes Sega FILM and decodes Cinepak separately; Neptune handles the SGX-assisted video presentation, matte, native VDP2 subtitles and fades. Audio is outside Neptune.

## Source navigation

- `src/platform/vita/neptune_renderer.cpp` — native SceGxm renderer and composition implementation.
- `include/lagi/vdp1_renderer.h` — model layout, rendering modes and prepare/submit API.
- `include/lagi/lagi_render_bridge.h` — published Azel model, transform, light and VDP1 UI command state.
- `include/lagi/lagi_point_light.h` — portable native point-light evaluator.
- `include/lagi/lagi_particle_color_adapter.h` — native animated particle Gouraud adapter.
- `shaders/` — texture, Gouraud, dithering, RBG/VDP2 and video programs.
- [ARCHITECTURE.md](ARCHITECTURE.md), [DITHER_PERFORMANCE.md](DITHER_PERFORMANCE.md), [RADAR_PRESENTATION.md](RADAR_PRESENTATION.md), [NATIVE_POINT_LIGHT_RESTORATION.md](NATIVE_POINT_LIGHT_RESTORATION.md) — implementation context and validation limits.

## Rules for extending Neptune

1. Keep Azel authoritative for command generation, scene/task state, light selectors and ordering.
2. Capture original transforms and color/palette metadata at the native boundary; do not infer from scene names or texture IDs.
3. Prefer SGX raster/sampling/composition to CPU reconstruction, but retain exact native math where required.
4. Distinguish *implemented*, *host/syntax tested* and *visually validated on Vita* in documentation.
5. Make improvements generic to field, town, menus and cinematics; preserve established point-filter exceptions and frame/resource ownership.

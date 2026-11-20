# Native battle effects and Berserk support

**Current review: 20 November 2026.** This is the working inventory of native effects that still need their original producers or shared draw services. [Disc 1 Berserks](BERSERKS_DISC1.md) contains the original unlock records and names.

## How the pieces fit together

The original game selects an attack, starts its native task and decides the target, motion, damage, camera and completion timing. Lagi provides the resource, allocation, lifetime and drawing services that task expects. Neptune renders the resulting VDP1/VDP2 commands. A registered factory or a successful damage calculation does not establish a complete attack.

The existing 30 Hz encounter measurements remain useful, but a scene that omits its authored effects has less work to render. Hardware comparisons need the full original workload.

## Shared services already recovered

**Chain and Prism Laser trails.** Original `BTL_A3 0609D9AC` submits segments through MAIN `0602E03C`. At `0609DA3E..56`, the VRAM allocation base is added to both the texture address at descriptor +4 and the palette address at +8. Azel's reconstructed adapter had relocated only the texture. The Lagi adapter corrects palette relocation without changing width, segment order, Gouraud colors, camera, timing or damage. `tests/battle_laser_palette_test.ps1` exercises the generated Draw code, but Chain/Prism appearance still needs a matched Vita capture.

**Reusable trail drawing.** `src/integration/lagi_battle_trail_draw.inc` reproduces the original BTL_A3_2 trail entries at `060A0EAA` and `060A0DE4`: per-segment width, optional four-corner Gouraud values, native segment order and texture/palette allocation offsets. This is drawing support, not a replacement for missing producer updates.

**Model, projectile and particle foundations.** Shared battle model initialization and targets, disc-backed projectile descriptors, fixed-point motion, particle frame tables, and effect submission are already present. The missing original per-attack child tasks must be connected to these services; a new Vita-only particle or attack system is not needed.

## Known unfinished native producers

| Family | Gap in the pinned Azel reconstruction | Shared service to recover or verify |
| --- | --- | --- |
| Blast Chip family | Simplified timer/update; missing original particle arrays, draw and light changes | Animated particles, model draw and effect-light lifecycle |
| Chain / Prism Laser | Core trail draw present; effects/lighting not fully verified | Trail packet publication, hit emitters and cleanup |
| Laser Storm | Missing homing-laser model initialization, trail history, ring emitter and Draw assignment | Homing beam model, trails and particle rings |
| Lightning Storm / Plasma Swarm | Lightning bolt Draw is empty; trail jitter and swarm motion/targets are incomplete | Native strip renderer, motion history and target traversal |
| Dragon Phoenix | Wing-fire child task is unimplemented | Native emitter creation, model/hotpoint attachment and ownership |
| Astral Phantoms | Phantom model/orbit children, target shuffle and damage sequence are unfinished | Animated model instances and native targeting |
| Phantom Slashers family | Projectile creation/rendering and some original target and frame behavior are incomplete | Shared model projectile, animation and hit paths |
| Energy Prism / Hunting Scythe | Trail updates, homing behavior, Draw and hit paths contain omissions | Trail/beam motion and native targeting |

This list identifies missing source functionality; each entry still needs original disassembly and disc-data checks before implementation. The precise producer state machine stays with Azel. We recover common routines where the original actually shares them instead of generalizing away differences.

## Render-side verification

A Draw callback must be restored before judging Neptune's output. Once it emits commands, check the actual Saturn VDP1 packet fields: texture and CLUT relocation, normal versus distorted sprite geometry, Gouraud values, mesh/stipple and color calculation, producer depth and submission order. Renderer changes should follow observed command differences, not an approximation of what a named attack ought to look like.

The shared battle light path uses `battleGrid_updateLightColorInterpolations`, `setupLight` and `generateLightFalloffMap`; some beam producers also publish a point light at their transformed source. Correct signed channels, coordinates, lifetime and full/lighting-only agreement require device verification.

## Related cinematic effects

The October E011 investigation identified a missing scheduled ribbon at `0605890C` (descriptor `060600C4`) and later shared event callbacks `0605809E`, `060557E0`, `06055872`. Follow-up work recovered the scheduler, quad/model/ribbon tasks and those registrations. The older initial-crash notes are preserved in [E011 transition triage](E011_TRANSITION_TRIAGE.md), with later hardware acceptance still distinguished from source tests.

## First useful test group

Lightning Storm and Laser Storm both exercise the original homing-laser/trail family, making them good shared-service recovery targets. Chain Laser, Phantom Slashers, Holy Sphere and Energy Prism cover relevant Disc 1 attacks with different rendering requirements. The order is about testing common primitives; it doesn't change attack selection or unlock rules.

The governing boundary stays simple: **Azel decides. Lagi services. Neptune renders.**

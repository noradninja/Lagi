# Camp handoff — 2026-11-24

## Resume point

Work-to-home handoff. Continue on `diagnostics/garil-cliff-current` (pre-documentation head `72e4a85df7617cca9002f0bec5c9ad14164e1fe8`). The separate `feature/venezia-audio-offload` branch is for audio work; do not mix unvalidated visual changes into it. `diagnostics/camp-gash-regression` retains the diagnostic history. Use atomic commits, user pulls/builds/tests on Vita. Azel decides; Lagi services; Neptune renders.

## Resolved: returning to Camp and talking to Gash

Hardware reproduction: load Garil Desert, leave the field via WORLD and return to Camp, select Gash to talk. Edge spins through a full rotation, falls through the floor, and no conversation menu appears. Reproduced on pre-Venezia baseline `af2b079`, with all 17 CMake Boolean options correct and Azel pinned to `4cebb24b91ed1499b8754cf992c2fc90dbfb044d`. This was not caused by the Venezia instrumentation or missing build options.

Evidence: `lagi(20261124-171554).log` and instrumented `lagi(20261124-173434).log` (user captures). Gash registration and rider/story state were present. Diagnostic commit `239809c` on the Camp branch recorded CampScript, CampNative and CampEdge. The faulty state was present before interaction: Edge's initial movement flags were FF, then 7F. Turning toward Gash activates scripted movement mode 04; flags 7B become 79 after rotation. Bit 0 remains set, so updateEdgeSub2 enters unintended scripted walking toward (0,0,0). X/Z remain unchanged and Y decreases by 86 raw fixed-point units per update. Script PC 060588F8, opcode 31, calls 06070946 (Camp NPC motion finished) with NPC 0 and waits indefinitely. 06070198 is the turn-toward-NPC callback. This establishes a gameplay-state fault, not camera projection.

Candidate commit `29df4b11a759968bcc7ed7d9c49a5e208615042f` explicitly clears mE_controlState and mF alongside mC in initEdgeNPC, before initEdgeNPCSub0 applies authored mode and shadow bits. User confirmed on Vita that this fixed the issue. The same anchored generated-source correction was verified present in CMakeLists.txt on both Garil and Venezia branches; temporary CampEdge/CampNative/CampScript instrumentation is not carried forward. Underlying reason the initial flags were nonzero remains unconfirmed. Do not modify the pinned Azel submodule or force script completion.

## Open visual issues (user-observed on Vita, not yet fixed)

### 1. Nighttime lighting

At night in Camp, meshes are black in both grayscale and colored light diagnostics. Trace the native day/night light setup, ambient/directional inputs, captured light state and refresh of static as well as dynamic geometry. Determine whether the original game uses a campfire point light and whether the native contribution is missing or lost in Neptune. Preserve authored lighting and darkness, rather than introducing arbitrary ambient brightness or global light changes.

Acceptance: nighttime meshes have the intended visible illumination, grayscale and colored diagnostic views agree with their respective lighting contributions, daytime Camp remains correct, and static objects and actors receive the appropriate native lighting. Compare to original Saturn capture if available.

### 2. Campfire sparks/embers

Rising ember quads are visible in diagnostic views but invisible in Full mode. Geometry is present, but missing or incorrect texture/sprite binding, palette, alpha, blend, or command interpretation is a hypothesis, not a confirmed cause. Follow the authored VDP1 attributes and source address through asset decode, palette, renderer submission and Full mode draw. Recover the authored appearance, not replacement artwork. Use point sampling for world particles/mesh sprites; retain stochastic filtering for UI/video only.

Acceptance: rising embers visible and animated in Full mode with correct transparency, palette and placement; no filtering or blending regression to other particles.

### 3. Campfire ground-circle blend changes when UI appears

Circle blends correctly against the VDP2 floor when no UI is onscreen. Whenever an onscreen UI element appears, the circle becomes opaque; when the UI disappears it blends correctly again. Trace VDP1/VDP2 composition routing and actual blend state with identical scene state, UI off/on/off. Inspect sprite-composite activation, pass classification, priority/color calculation, blend-state restoration and target selection. Do not assume this is a texture alpha bug or globally change UI composition.

Acceptance: the circle maintains its authored translucent appearance throughout UI transitions, without changing UI rendering, VDP2 floor, other scene sprites or actor rendering.

### 4. Ground-circle height relative to actor shadows

User requests a very small downward placement change so actor shadows appear above the blended circle. Historical camp adjustment of -0.015 was visually correct in an earlier build; verify the current code and coordinate sign before choosing any additional offset. Trace actual geometry, depth test/write and composition ordering. Keep the circle immediately above the floor while below actor shadows, avoiding z-fighting or burial. If pass ordering is the true cause, address it rather than using a large depth bias. Scope any offset to the authored circle, not all ground decals.

Acceptance: Edge/Gash/dragon shadows remain visible above the circle where they overlap, circle stays blended and attached to the floor during camera motion, and no UI-dependent opacity or z-fighting.

## Next steps and test matrix

Implement in four atomic commits in the order above, with source/data evidence and regression tests where practical. Keep each commit independently reviewable. No visual implementation has been committed as part of this handoff.

Test on Vita using the same save and Camp setup:
- Garil -> WORLD -> Camp -> talk to Gash (regression guard).
- Camp daytime and nighttime; Full, grayscale light and colored light modes.
- Campfire close-up: embers rise, point-sampled and transparent.
- Ground circle with UI off, UI on, and UI dismissed.
- Walk actors across the circle and rotate camera; inspect shadow ordering.
- Exit/reenter Camp; verify no repeat of stale state, blending or light issues.
- Confirm no new performance regressions and no changes to audio.

Capture logs and before/after video, including build SHA. Only integrate the verified visual fixes into Venezia after hardware acceptance.

## Home build

Use PowerShell and the home layout (E:\dev\Lagi, E:\dev\VitaSDK, E:\PSVITA\sdk\host_tools\bin\psp2cgc.exe). Check local edits before switching. Pull `diagnostics/garil-cliff-current`, verify HEAD and pinned submodules, and use the existing correct Rider release profile or configure a fresh release directory using [BUILDING.md](BUILDING.md). All 17 Boolean options there were verified on the failing baseline; configuration is no longer the suspected cause. Keep the existing full-resolution, optimized renderer, world map and native battle defaults. Do not resume the Venezia implementation until the chosen gameplay baseline is sound.

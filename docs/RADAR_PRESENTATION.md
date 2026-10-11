# Radar-only presentation correction

Generated-source instrumentation tags the command range emitted by native
fieldRadar_draw. Ownership resets each frame; no upstream files are edited.
Only these commands receive square integer scaling and right-edge anchoring.
Native positions/slide offsets and gameplay visibility are preserved.

At 960x544 the frame is 160x128 and the orb 96x96 (2x their native textures).
At 480x272 they are 80x64 and 48x48. Native X=176 anchors to the right edge;
Y=-112 anchors to the top. This supersedes the orb-only scaling described
in earlier architecture/flight notes. Other UI keeps its existing transform.

The orb remains point-filtered, with texture-edge UVs 0..1 instead of half-texel
insets that compressed the source span and produced uneven checker repeats.
Title, Cinepak and non-radar dithered filtering remain unchanged.

Native command 4 now renders the RGB555 red altitude polygon, previously skipped
by the UI renderer; command 6 renders the direction line. Azel determines fill
height and gauge enablement. At minimum altitude it legitimately emits no fill.
Dedicated per-submission vertex slots prevent line/fill overwriting one another.

Validation: renderer/runtime Vita compiler syntax checks and CPU/source grid
checks passed. No VPK build or Vita visual test performed. Verify checker cells,
frame alignment, upper-right placement, hide/show, and fill at several altitudes.

User hardware acceptance: in-game saving and loading stations work correctly
(2026-10-11). This does not mark all menu presentation/restoration work complete.

Hardware regression after the first radar change: placement is accepted, but
the frame disappears and L-button destination selection crashes. Source audit
found the solid radar line/fill branch left the color vertex shader active for
subsequent textured sprites. Restore both textured stages and the identity
vertex uniform after each solid submission; regression guards cover that
handoff. This corrects a proven state leak, not a proven complete explanation
of the L-button crash. Retest frame visibility and L hide/show on hardware;
if the crash persists, obtain the matching log and full dump.

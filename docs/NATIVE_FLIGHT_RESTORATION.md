# Combined navigator, radar and flight-effect restoration

Branch: `feature/menu-save-load-restoration`. Hardware reference: `8b9cf45`
still failed on R-release. The complete matching dump identifies native
dispatch `06073120`; the previous build only restored `06073172`.

## Native navigator

- Restore `06073120` by calling the existing native `updateMultiChoice` task.
- Restore `06072F3C`: wait for task deletion, map filtered choice back to the
  original destination, update global text slot 2/selection, then free styles,
  choice data and the filtered pointer array. Cancellation leaves selection.
- Restore the nonnegative default-choice highlight branch (`06079234/060792BA`).
- Resolve raw Saturn string EAs rather than casting them to host pointers.
  Read the original deactivate string through `06095A28`, not an invented label.
- Keep the BE pointer table and copied/rebased strings in one memory-file view.
  Release it from native dialog Delete, including parent-driven teardown.
- Correct native dialog cancel to `-1`: original `0601DCA2/0601DCA6` stores that
  value; reconstructed task2 incorrectly returned entry zero.

The constructor, cursor, wrapping input, box drawing and native task ordering
remain Azel's. Nothing disables navigator or fakes a successful dispatch.

Final cleanup audit also repairs the native task-heap service. The old Vita
free helper deleted storage without unlinking its header; the next allocation
walked a stale task-list node. Original `060151FE/06015234` searches ownership
and updates that link before freeing. Restore this contract for head/middle/tail
nodes. The lifecycle test now uses the actual heap service, retaining a radar
destination allocation through repeated filtered-choice allocation/free cycles.

## Radar

Original `0607281E` restores object-marker distance/range checks, negative-yaw
rotation, native X/Z projection constants, `06093F54` colors, and `060726DC`
size/altitude shapes. Original `0606E750` visits both pending and active LCS
target lists, transforming flag `0100` positions through `cameraProperties2.m28[0]`.
Selected destination `06071EC4` emits its limited-angle bearing line and marker.

Command ownership now accumulates per frame/slot across radar and LCS tasks;
one later marker range cannot replace frame/orb ownership. Keep the previously
accepted right anchor and square integer grid. Radar orb remains point sampled.
The existing original initializer repair retains the 24-to-10 orientation
segment, positive center offset, and native hidden/slide animation offsets.
No other UI element receives the radar repositioning transform.

## Scene occlusion

Projected particle and ray producers now publish actual endpoint view Z.
Renderer maps it with the same perspective depth function as the 3D scene,
uses LESS_EQUAL and depth writes for those explicit scene commands, and retains
ALWAYS/no writes for HUD/menu commands. Slot validity resets each frame.
Do not reinterpret native packet bucket keys as GXM depth.

Field mesh phases previously used unconditional overdraw/no depth writes.
Both ordinary and subdivided field mesh paths now compare/write scene depth;
stipple-discarded fragments leave holes. Town ordered-shadow behavior and mesh
sampling/shaders are unchanged. Waterfall/particle occlusion and performance
must be verified on the device; host checks cannot prove framebuffer behavior.

## Collection completion radial burst

The user clarified this means radial lines around the dragon on collection,
not the laser joining an item or a targeting box.

Original `0607BAE4` writes the randomized radial distance at offset `34`, which
is the Z component of the `2C` local vector. Reconstruction stores that distance
in an additional named field, then zeroes the vector's Z. Restore the local Z
from that native value; preserve original random angles, duration and opacity.
Original `0607BB9E/0607BC06..52` calls the seven-argument `0602F610` service:
dragon center and rotated radial endpoint, widths `{0, scaledOpacity}`, texture
`base+06B0`, size `031E`, palette `base+1780`, mode 8, and BE Gouraud data at
`06094F64 + color*8`. This replaces the missing Draw stub.

The typed two-width service now restores matrix transformation, near/far
clipping, native 12-bit direction and sine/cosine corner math, VDP1 packet and
Gouraud emission, and endpoint depth. The old two-argument billboard service
was an unconditional no-op; it is now the typed internal adapter for this call.

Degenerate scene sprites require pixel coverage, not zero-area GXM triangles.
[Sega VDP1 manual 7.6](https://www.infochunk.com/saturn/segahtml_en/hard/vdp1/hon/p07_60.htm)
explicitly retains one pixel when all four vertices coincide. The renderer
retains that coverage and paired-edge line collapses for explicit scene quads;
ordinary quads/triangles and HUD geometry are unchanged. Paired-line coverage
is a renderer mapping inferred from the inclusive native raster, not a claim
of a complete pixel-exact VDP1 rasterizer. No extra burst task or invented
effect timing/color/radius is introduced. Non-radar native CMD6 lines also no
longer disappear through the texture-only fallback.

## Verified checks

- `native_menu_pipeline_test.ps1`: actual production CMake source transformations,
  not hand-recreated replacements, for FLD_A3, radar, LCS, dialog and VDP2;
  patched-source plus navigator/radar/runtime/particle-service Vita syntax.
- `navigator_lifecycle_test.ps1`: execute actual navigator service bodies with
  platform/task mocks and the actual production task heap; open/wait/select,
  64 cancels/reopens, deactivate, forced teardown, BE string rebasing,
  head/middle/tail unlinking and view/style/choice lifetime checks.
- `collection_burst_service_test.ps1`: execute actual two-width service bodies
  with math/platform mocks; texture/size/mode/Gouraud/depth, nonzero radial
  geometry, near/far rejection and crossing-plane clipping. Its thin triangle
  is retained without unnecessary expansion.
- `native_flight_geometry_test.cpp`: 4,363 marker/depth/degenerate-coverage cases;
  compare perspective Z with the renderer matrix, preserve normal geometry.
- `glyph_clear_test.ps1`: corrected native glyph ownership/origin, 15,876 model
  cases and patched Vita syntax. An additional extracted-function host test
  executed all 15,876 cases, checked exact four-cell VRAM/ownership clearing
  and one reference release. Full load-to-black remains unaccepted.
- Neptune renderer Vita syntax and `git diff --check`.

These are host/source/syntax checks, NOT full Vita linking, VPK compilation,
device execution, a matched performance capture, or visual acceptance. The
local full-project configure attempt stopped at an unavailable checked-out
ThirdParty SCSP source; the dedicated harness verifies the actual adapters
without changing upstream/disc files. The user performs the full local build.

## PowerShell build and combined hardware check

After the pushed checkpoint is identified in chat:

```powershell
Set-Location E:\dev\Lagi
git switch feature/menu-save-load-restoration
git pull --ff-only
git rev-parse --short HEAD
cmake -S . -B build-fullres
cmake --build build-fullres --parallel 32
```

Check R release, choose a destination, cancel, reopen, deactivate, and reopen
again. Rotate/accelerate near items to inspect orientation/object markers and
frame/orb/altitude alignment; exercise navigator hide/show. On the same route,
check waterfall and particle occlusion plus the radial burst when an item is
collected. Confirm radar stipple stays point-filtered and town shadows remain
unchanged. Supply the matching log/video/dump if a failure remains; preserve
that build's unstripped ELF. `[NativeRayCoverage]` is bounded to eight messages
and reports collapsed scene sprites, not proof of the collection burst.

Still open: full save-load black fade, visual radar risk-palette fade, full
unfinished native menu pages and hardware acceptance of this checkpoint.
Do not mark the overall in-game-menu objective complete from these checks.

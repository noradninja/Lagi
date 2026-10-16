# Menu tile decode cost and animation rate

## Hardware rejection and rollback — 2026-10-16

The user reports menus dropped from 20 FPS to 15 FPS with this cache; Dragon
positioning is now confirmed correct. The cache implementation and its tests
are removed. The renderer is restored exactly to its pre-cache code, while
the independent preview-root fix is preserved. The historical design and host
results below are not evidence of a Vita performance win. A fresh log of the
regressed build was not yet available at rollback time, so collision thrashing
or lookup overhead remain hypotheses. Rollback performance needs hardware
verification. No animation-counter multiplier is added.

## Historical investigation (cache now reverted)

The2026-10-16 hardware report shows20FPS menus, slow panel navigation and a
miscentered Dragon preview. The updated Downloads lagi.log (13:41 local,
1,317,479 bytes) contains55 ScenePerf rows with compose>20ms: mean compose
35.641ms (35.350..35.897), mean total renderer47.414ms (46.315..47.954).
Presentation outliers show3 VBlanks/about50ms for this menu path. This is
evidence of substantial CPU menu-composition cost, not a GPU-only bottleneck.

Native cadence changes are logged (request1 for menus; the Dragon preview
subpage also authors request2). No blanket clock multiplier is introduced.
Read-only Disc1 MAIN06019346 confirms the save-panel opening uses offset120hex,
nine movements of-20hex and native delay countdowns, matching the port's opening
routine. Slow rendered updates therefore slow these per-update animations in
wall-clock time. This does not establish acceptance of every reconstructed
menu timer or close path; it specifically rejects an unsupported universal2x
step multiplier as the proposed performance fix.

## Change

Status-menu4bpp and Save-menu8bpp backing pixels previously repeated character,
flip, palette and RGB conversion work for every pixel of both planes every
frame. `MenuTileCache` retains the native16x16 decoded RGBA result by complete
pattern word and format. On the first touch of a tile in a new immutable
published frame it compares its exact128/256 character bytes and32/512 palette
bytes; unchanged sources reuse pixels. Later touches in that frame only select
the cached pixel. Source changes re-decode the tile; key collisions are misses,
never accepted as identity. Bounds/partial spans preserve transparent output.

The cache is render-owned,1024 fixed entries/about1.8MiB, with no per-frame
allocation or producer pointer retention. Map/scroll changes select new keys;
scroll does not invalidate unchanged character data. Glyphs remain a separate
foreground pass. Map NBG2 composition, native priorities, filters/dithering,
palette/VRAM ownership and GPU draw count remain unchanged. Published input
VRAM/CRAM is immutable until the next backing pass, where `beginFrame` resets
the source-validation epoch. The output texture's existing lifetime is retained.

`ScenePerf` now appends `menuTiles` (decoded entries this backing frame).
An idle unchanged menu should normally approach0; animated source bytes and
cache collisions can legitimately cause misses. Lower counts do not prove
lower frame time: compare compose/render/presentation intervals on hardware.
First-use decoding still costs work, so entry/navigation spikes also need testing.

## Checks and limitations

Host tests compare219456 cached and uncached production status/save pixels
across frames, patterns, palettes/flips, signed scrolling and address bounds.
Separate checks cover exact character/palette invalidation, unchanged-frame
reuse, one source validation per touched key per frame, and forced collisions.
The existing Map NBG2 decoder/priority/blend tests and renderer Vita syntax
also pass. No new on-device timing or appearance is claimed. The supplied
capture is the baseline, not a measurement of the new cache.

The separate preview-root commit supplies a native view for the already
implemented local-origin correction; it still needs a screenshot against the
circle. Do not claim centering or navigation speed is fixed from host checks.

PowerShell build/test handoff:

```powershell
cd E:\dev\Lagi
git switch feature/menu-save-load-restoration
git pull --ff-only
cmake --build build-fullres
```

Repeat the main-menu opening, Save sliding panels and Dragon preview in the
same session. Supply a fresh log and preview screenshot, checking menuTiles,
compose/render cost, FPS and actual panel duration. Do not change resolution,
filter switches or animation counters during this comparison.

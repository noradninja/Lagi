# Menu tile decode cost and animation rate

## Row raster optimization (2026-10-16, device test pending)

The fresh 153,386-byte capture confirms the GXM selector at 60 FPS, including
selection changes. The user still observes ordinary/save menus at 20 FPS and
Map/Dragon at 15 FPS. Its two heavy ScenePerf samples show backing decode
30.540/30.771 ms, text decode 4.211/4.297 ms, transfer 1.474/1.442 ms and total
render 48.900/48.779 ms. These nested costs must not be summed with composition
or renderer again. Both have 247 polygons; without explicit menu state labels
the samples do not individually identify every menu tested.

`MenuRowRaster` expands the immutable snapshot's 2,048 CRAM entries once per
backing/text pass and resolves map/character/palette metadata once per run of
up to eight pixels. The inner dot loop only reads source dots and expanded
palette entries. It handles status 4bpp flip-capable names, save 8bpp no-flip
names, NBG2's four planes/SCN/CAOS/window, and the separate 8x8 text plane.
Original NBG2 priority and RGB555 color calculation remain unchanged.

This is a CPU raster optimization, not a GPU tile decoder. Existing GXM
surface submission, dithering and source-raster dimensions remain unchanged.
Every pixel is rebuilt from the current published snapshot: no tile hashing,
cross-frame decoded cache, stale-resource assumptions, skipped animation or
counter multiplier. Additional stack storage is about 12 KiB (palette and
three rows), without heap allocation. The cached scratch-to-GPU copy remains.

Production-body host tests compare 8,110,080 pixels with the original decoder,
including all 65,536 pattern words for both backing formats, unaligned runs,
signed wrapping, changed VRAM/CRAM, NBG2 window/blend/SCN/CAOS combinations,
and backing/text independence. Existing NBG2 tests additionally cover 32,768
RGB555 blends and 3,584 priority cases. These prove tested output equivalence,
not Vita speed. The final full-resolution VPK build and the accepted selector/
font regression checks pass with both dithering flags enabled. The next device
test must compare the same menu screens and
panel/Map scrolling with no configuration changes.

ScenePerf now appends `menuId`, `saveLayout`, `menuNbg2` and
`menuRaster=rows-v1` to the existing heartbeat (no additional logging loop).
Capture at least 10 seconds each of ordinary menu, save menu, Dragon and Map,
including movement, to obtain several timing samples per screen.

## Current hardware report (2026-10-16)

The user confirms that field-map viewing, scrolling and exiting work. The
current map and Dragon screens run at 15 FPS; other menus run at 20 FPS.
These are device observations, not host-test estimates. The accepted pre-Azel
disc selector/font/layout work is independent and remains approximately
60 FPS. Do not alter native animation counters to conceal menu rendering cost.

The current Downloads log is only 592 bytes and stops at the disc selector;
it contains no ScenePerf samples from these menus. A fresh same-session log
must include ordinary menus, Dragon and field Map (including scrolling),
with several seconds in each. Compare menuBackingDecode, menuTextDecode,
menuTransfer, compose, cpuprep, renderer and presentation intervals. The
older CPU-composition measurements below remain historical evidence, not
measurements of the accepted final tree. Map's additional NBG2 composition
and Dragon's actor rendering require separate attribution.

## Hardware rejection and rollback — 2026-10-16

The user reports menus dropped from 20 FPS to 15 FPS with this cache; Dragon
positioning is now confirmed correct. The cache implementation and its tests
are removed. The renderer is restored exactly to its pre-cache code, while
the independent preview-root fix is preserved. The historical design and host
results below are not evidence of a Vita performance win. A fresh log of the
regressed build was not yet available at rollback time, so collision thrashing
or lookup overhead remain hypotheses. Rollback performance needs hardware
verification. No animation-counter multiplier is added.

## Cached CPU staging (2026-10-16, hardware pending)

The14:15 log (136,487 bytes) and user report confirm rollback menus at20FPS.
Its one menu ScenePerf sample reports background decode30.678ms, text
decode4.702ms, CPU submission0.034ms, total composition35.492ms and renderer
47.341ms. This sample locates the cost in CPU pixel composition, not GXM
command submission; it does not isolate decoder arithmetic from memory writes.

Both pixel surfaces came from `probeGpuAlloc`, which allocates
`SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE`. The menu compositor now writes
the same pixels into one aligned, module-owned CPU scratch surface, then
copies315,392 bytes contiguously into the selected GPU allocation before
submission. Scratch costs308KiB, is never GPU-mapped or sampled, and is fully
written/cleared on every use. There is no decoded-tile cache, source hashing,
reuse key, skipped frame, animation-rate change or added GPU draw. Backing and
text retain separate GPU allocations, so reusing scratch for glyphs cannot
overwrite the queued background. The existing uncached GPU destination needs
no new CPU-cache flush; cached scratch is only read by the CPU copy.

`menuTransfer` reports the summed CPU copy cost separately, in microseconds.
`menuBackingDecode`/`menuTextDecode` now measure composition into scratch.
All remain nested in `compose`/`cpuprep`; do not add them a second time.
The change is a measured-path optimization hypothesis, not an accepted
performance result. A new same-menu hardware capture must compare decode
plus transfer, total renderer cost and actual presentation intervals.

Full package verification subsequently passed for source commit `b6ccf07`:
359-step clean local Ninja build, GNU VitaSDK15.2.0 and Sony psp2cgc,
`LAGI_FULLRES=ON`, `LAGI_STOCHASTIC_FILTER=ON`, `LAGI_DITHER_LOOKUP=ON`.
The pinned Azel commit remains `c52329fb257561abff05531a8335e34137d67098`,
with no tracked source differences. Output `build-menu-verify/Lagi.vpk` is
1,485,991 bytes, SHA256
`2782779ADAB843F313AA23F794D62E8A1919B24EEEFB9E3309A611A5DF50B9C5`.
Packaged `sce_sys/param.sfo` was read from the VPK: TITLE=STITLE=`Lagi`,
TITLE_ID=`LAGI00001`, APP_VER=`00.40`. Existing shader-shadowing and native
array-comparison warnings remain; no compiler/link/package error occurred.
This verification build has zero locally captured DSP AOT programs, so it
is not a substitute for the user's existing audio/performance setup. It was
not installed, deployed or run on hardware. The verification directory is
retained locally as an untracked build artifact, not committed.

`tests/menu_surface_staging_test.ps1` compiles the production composition and
copy block against the original direct-output algorithm. It compares1,261,568
pixels over status/save/Map configurations, signed scrolling and changing
VRAM/CRAM, including transparent text clearing and independence of the two
GPU destinations. These host checks and Vita renderer syntax pass; they do
not model Vita cache/bus performance or GPU execution.

## Historical investigation (cache now reverted)

Follow-up log received at14:07 (403,723 bytes) contains the cache's
`menuTiles` field, so it is a cache-enabled build, not proof of rollback
performance. Seven menu-like ScenePerf samples (compose>20ms) average
36.651ms composition,48.735ms renderer,42.852ms CPU preparation and4.098ms
GXM finish. Every one reports52 decoded tiles. Presentation outliers show
four VBlanks (~66.733ms), confirming15FPS. Compared with the earlier capture,
the renderer costs about1.3ms more; scenes/captures are not a controlled
benchmark. Extra CPU cost near a presentation boundary is consistent with
the discrete FPS drop, not proof of a doubled animation counter.

Added bounded ScenePerf fields (microseconds): `menuBackingDecode`,
`menuTextDecode`, `menuSubmit`. They time only the two CPU pixel-composition
sections and their GXM submission, reset each renderer frame, and reuse the
existing60-frame log heartbeat. No per-pixel clocks/logs or new upload/pass
are introduced. `menuSubmit` is CPU submission cost, not GPU duration; the
menu texture uses shared GPU-visible storage, not a separately timed upload.
Allocation/initialization is outside the decode sections. These nested costs
must not be added again to `compose` or `cpuprep`.

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

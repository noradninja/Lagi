# Native Map menu restoration — in progress

Current status: the newer Vita child enables entry/exit only, not a finished
map viewer. See `FLIGHT_MAP_MENU_BRINGUP.md`. Historical checkpoints below
that say the constructor is disabled describe the earlier state.
This work is distinct from fixed pickup target flags and shared item ownership.

## Live map context checkpoint

`lagiAzelResolveCurrentMapContext` now implements MAIN `06016C54..06016D56`
and binds the result at Map child entry. It reads the native scene-class bits
`readPackedBits(0,2)`, current field/subfield and dragon position/yaw. Ordinary
fields retain their actual subfield and XYZ. Fields `0F` and `12` use the live
COMMON zone lookup `2161F4`, zone offsets `216218`, and subfield offsets
`216290`, matching native `0602F87C`/`0602F8E4`. The transformed zone, not the
raw subfield, selects the descriptor. Zone values at least15 use the native
X translation and `Z=-Y-subfieldOffsetZ`, returning slot15. Other zones add
both native X/Z offset pairs with 32-bit wrap.

Original subfields `0/6/9/17/1A` derive heading through identity,
Y(yaw), X(pitch), then native `atan2_FP(m02,m12)`; other subfields retain
yaw. This order is verified from MAIN `06035E68` (Y, columns0/2) and
`06035DC8` (X, columns1/2), not inferred from filenames or screenshot shape.
The service reuses existing native matrix/trigonometry services. Map X/Z
conversion uses signed division by65536 (truncation toward zero), Z adds400hex,
and final map coordinates are signed16 like the original work area.

The COMMON slot reader is separately bounded for addresses/strings; native
field/zone state supplies its selected index. UI callers still use the existing
count-checked wrapper. The special-field byte lookup is bounded at its adjacent
zone-offset table (24hex entries), avoiding upstream's truncated sixteen-entry
copies that cannot cover native subfields17/1A.

The Map child owns the captured context and emits one `[FieldMapContext]`
entry with descriptor, selected slot, coordinates, heading and SCB/PNB names.
The newer resource owner loads those real sources with rollback
(`NATIVE_MAP_RESOURCE_OWNERSHIP.md`); a Map image is not yet promised. Normal/absent context,
36 special routes, conversion direction/truncation, rotation/atan binding and
invalid-state guards execute in `tests/map_context_test.ps1` against the actual
service with mocked native services. Existing entry/ownership/pattern/marker
and loader tests pass, as do Vita syntax checks of the service and Map child.
No full linking, VPK or hardware result is claimed.

Next required integration remains the native parent selection/mode flow and
owned resource handoff to the compiled view child and NBG2 compositor. The
view child now contains pan/fade and player-marker services, but is not called
by the placeholder parent (`NATIVE_MAP_VIEW_TASK.md`). The
existing placeholder is not the requested final menu, and this checkpoint does
not count the Map viewer as complete.

## Verified original task and data

COMMON menu dispatch `0020DCD0` has Map constructor `06017B84` at `0020DCE4`.
The constructor creates task definition `0020E8F4`, size 40 hex. That definition
contains Init `060179CE`, no Update, Draw `06017A68`, Delete `06017B74`.
Init has ordinary status-menu entry versus standalone entry paths: the former
scrolls layers for 16 frames; the latter saves vsync and sets it to one. It
calls `06016D88` for page availability/current selection, `06016E64` for map
resources, and creates cursor definition `0020E8B4`. Delete restores vsync only
for the entry mode that changed it. Draw states 0..4 cover opening wait,
selection setup, native mode-specific input, closing and the final wait.

`06016C54` derives field context from actual field/dragon state, including
special field indices 0F/12 and subfield cases 0/6/9/17/1A. Its descriptor
lookup at `06016D36..46` reads field table `0020E848` (23 BE32 pointers), then
a subfield pointer and 12-byte descriptor. No descriptor is a valid null result,
not permission to fabricate a map from radar geometry. Descriptor +0/+4 are
authored SCB/PNB filenames; +8..B are original presentation bytes.

`lagiAzelReadFieldMapDescriptor` now reads that native hierarchy from the live
COMMON image, validating address ranges, filename termination and the caller's
authoritative subfield count. It returns borrowed filenames and the original
four presentation bytes without inventing their meaning. It does not replace
the context-selection computation or implement a page.

Host production-reader execution against the user's original read-only Disc 1
COMMON finds 13 available first-subfield descriptors across all 23 field slots.
Field 1/sub 0 resolves descriptor 0020E4B4: MAP_A20.SCB / MAP_A20.PNB with
bytes FF/12/04/1F. Field 2/sub 0 resolves 0020E4C0. Null table slots,
out-of-range field/subfield, null input and truncated image are rejected. Vita
syntax passes. No game assets or upstream sources are copied/modified.

## Remaining required work

Map object markers now have a native drawing service: current-field/subfield
record+4 supplies the16-byte list; getBit566 controls visibility; signed XYZ
16.16 coordinates become map X/truncated world Z+400, then native viewport
clipping and drawMenuSprite with palette760. Verified MAIN43260/68 definitions
are SRCA23F4/23F0, SIZE0108, offsets(-4,-4). Types2/3 have no verified sprite
definition and are deliberately unsupported, not assigned fabricated graphics.
COMMON bounds and terminatorbit4 prevent overruns. Actual host service tests
cover both emitted sprite types, saved-flag rejection, clipping and unsupported
types; Vita syntax passes. Player/orientation marker is a separate routine,
and the native view task must still call this service. Device output unverified.

Shared field/subfield record layout is confirmed by both native consumers:
record+0 is the exploration rectangle pointer, record+4 is the map marker
list pointer (060125CE), and signed exploration base is at record+8.
MOV.W displacement4 at060124EA scales by two; an earlier interpretation
mistook it for byte offset4. Exploration binding and fixture are corrected,
with a distinct nonzero marker pointer proving it is not read as a flag base.
Marker records themselves are16 bytes: XYZ fixed-point longs, flag word+C,
icon/terminator byte+E. Native iteration tests bit4 after advancing to the
next record. Full marker decoding/emission is still pending.

View-origin calculation 06017228 uses the signed center low words and NBG1
backing scroll: X=(centerX-2)*32+backingX-60,
Y=(centerY-2)*32+backingY-20, stored as native signed16 values. The service
lagiAzelSetMapViewCenter applies that pair to NBG2 live layer state, preserving
the backing layer. Actual host service tests check calculation and target layer;
Vita syntax passes. No output-resolution scaling or guessed center is inserted.
Native child state still needs to drive this service and its eight-step pan.

Source-half investigation resolved a loader error: literal06016EC8 is
0024C5A4 for the short-PNB fill, but literal06016FE8 is0024BDA4 for the actual
PNB load. Thus a PNB<=800 prefills the SECOND half with0238 and loads at the
FIRST half. A longer PNB spans both. The service now requires capacity1000
for short PNB, fills offsets800..FFF and preserves untouched first-half bytes.
Earlier loader notes implying first-half padding/fill are superseded. All20
threshold/fill cases now verify the distinct halves, including capacity rejection
before mutation; Vita syntax passes. The task can own one contiguous1000-byte
source allocation and pass its two authored halves to pattern presentation.

Marker viewport clipping from 06016F90 now preserves native scroll subtraction
and inclusive bounds X60..120, Y(20-offset)..(E0-offset). Rejected markers do
not overwrite the output coordinates. More than one million host cases cover
all four edges and signed scroll/vertical offsets; Vita syntax passes. Marker
sprite emission and its actual source position/rotation still require task
integration. Source-half initialization remains under investigation: original
view setup uses separate DRAM halves, while the PNB loader targets only the
second address; do not manufacture the first half from the second without
tracing its producer.

lagiAzelPresentMapPatterns now connects conversion to native current-field
ownership and exploration services and writes both original planes at VDP2
70000/70800, queried from Y15/Y31 respectively. Sources remain separate,
caller-owned authored halves; do not assume one PNB load initializes both.
Both input spans and context are validated before writes. Host tests execute
the complete service/converter/COMMON-lookup chain with mocked inventory and
flag storage, checking explored words, owned/unowned hidden words, distinct
source halves, outside guards and rejected short spans. Vita syntax passes.
Native view task must still supply the actual source buffers and context,
advance draw/input/fade states and render its marker; this is not an enabled page.

Native exploration service 0601249C is now bound by lagiAzelMapCellExplored:
current-field COMMON A388 -> caller-selected subfield -> exploration record
and signed rectangle. It rejects points outside the authored width/height,
computes dy*width + signed base + dx, then uses existing getBit566, preserving
native threshold1000/subtract566 and saved MSB-first flags. Caller supplies
authoritative subfield count; no new flags or radar-derived visibility exist.
Actual service host tests cover hierarchy lookup, distinct rows, all rectangle
edges and invalid subfield without flag reads. Vita syntax passes. The converter
still needs its caller/context hookup in the unfinished native view task.

Current-field ownership now binds native 06012554: read the signed item from
COMMON 20A650 indexed by the actual field task's current field, reject negative
entries, then call existing mainGameState.getItemCount. It does not assume every
field uses B2 and does not grant ownership. Host tests execute the actual binding
with all 23 field slots, owned/unowned counts, negative entries and invalid/null
field guards; Vita syntax passes. Exploration-bit lookup remains separate.

Original 060172B8 pattern conversion now operates on real PNB words: each
half processes 16x16 2x2-word blocks, with 0x40-byte inner-row stride and
0x80-byte block-row stride. Exploration queries use ascending X and descending
Y (firstY15/31 for the two halves). Explored blocks add4000; unexplored owned
map blocks add7000; unexplored unowned blocks fill0238. Additions wrap as
native 16-bit words and output remains big-endian. Ownership/exploration
callbacks must be bound to original game state, not invented visibility.
Host tests cover 4096 output words across both halves and ownership states,
callback order, word wrap, blanking and buffer guards; Vita syntax passes.
Map task hookup, exploration service binding and rendered output remain pending.

Map-view window setup 060173AA..06017406 now has a native service:
signed caller-owned scroll coordinates derive window0 start (60-x,20-y),
horizontal doubled endpoints with inclusive end and 192-line vertical span.
CCRNB preserves upper bits and sets low ratio31; map priorities become
PRINA0304/PRINB0705/PRIR0 and pending registers are marked dirty. Host execution
checks signed scroll, exact inclusive endpoints, preserved ratio bits and
priorities using the actual service body. Vita syntax passes. Caller scroll
ownership, pattern conversion and child task integration are still required;
this does not establish a rendered or accessible Map page.

Map-view child definition COMMON 20E8E4 has null Init/Update, Draw 060174C8
and Delete 0601763C, allocated size28. Its own phases0..7 include setup,
16-frame NBG2 calculation-ratio transition, active draw/input, closing,
eight-step scrolling and return. The parent mode1 follows child completion,
not a direct parent button shortcut. Full child implementation remains required.
Delete is now implemented as lagiAzelRestoreMapViewPriorities: pending PRINA
0304, PRINB0700, PRIR0, dirty1. Actual production cleanup executes in the entry
host fixture and handles absent pending registers safely; Vita syntax passes.
This service is not connected to a native child task yet.

Selection decisions from 06017762..060178A2 now live in lagiAzelMapSelect:
native input bit1 cancels before confirm bits6; confirm rejects availability0
with sound5, otherwise selects original mode1/2 with sound0. Vertical bits30
wrap two slots, skip absent -1, retain disabled0 for rejection, and request
sound10 plus old/new highlights only for availability1. The future native task
must apply sound/cursor/message/highlight services and mode constructors;
this helper is not yet hooked up. A bounded fallback prevents an infinite
skip loop only for malformed both-absent state. Tests execute specific native
branches plus 1728 cursor/availability/input cases. Vita syntax passes.

Lifecycle routing from 06017A68..06017B72 is now modeled by
lagiAzelAdvanceMapLifecycle for later task integration. Opening waits 16 draws
before selection setup; phase2 routes modes 0/1/2 to 06017762/060178A4/06017978.
Standalone selection setup chooses 06017718 only when availability slot0 is 1,
otherwise 0601770A. Menu close requests 16-frame scrolling and decrements on
the same draw (first remaining count 15); standalone close requests fade/finish
immediately. Mode-specific native services still own input/state mutation.
Host tests verify these timings/routes and Vita syntax passes; the helper is
not a replacement page and is not connected to the still-disabled constructor.
Earlier input addresses 06017882/06017964 are superseded by direct tail-branch
targets verified above. Delete restores saved vsync only in standalone mode.

`lagiAzelSetupMapEntry` now implements 06016B04 through existing native
services. 06015B8A/06015BB4 resolve to setVdp2LayerScroll and reset auto-scroll;
06034AAA is fadePalette, not a copied palette backup; 0600FC14 is the existing
compressed-graphics unpacker. Both modes unpack COMMON E3D4 to VDP2 71000;
standalone additionally resets all four layers, clears the native text area
and requests neutral C210 fades on both channels. Normal menu entry retains
its animated layer state. Host execution verifies exact call order for both
modes and null-resource guards with mocked services; Vita syntax and loader
regressions pass. Constructor/task/display/navigation remain unconnected.

Further original-code trace distinguishes entry setup from map image loading:
060179DE calls 06016B04 with R4=0 on the normal menu-entry path; standalone
06017A34 calls it with R4=1 before changing saved vsync. Nonzero setup invokes
06015B8A for four layers (last scrollY=100), 06015BB4, clears text rectangle
(0,22,2C,1C), and performs two palette operations through 06034AAA with C210.
Both entry paths finally pass COMMON 0020E3D4 and destination 25E71000 to
0600FC14. These dependencies must be resolved before implementing this setup;
do not mistake 06016B04 for a self-contained graphics backup or copy the Books
backup spans into Map without native evidence.

Map message helper 06016B6C configures rectangle (8,38,1C,4), clears it,
selects the existing native palette value at 0604FD5A, resets string-context
flags only for nonnull text, then invokes 06025334. 06016BAA chooses authored
COMMON text at 20D9B9/20D9CF/20D9DE/20D9EE based on mode and availability,
including live text cursor placement. Preserve those choices rather than
inventing labels. This trace does not enable rendering or prove page behavior.

Native loader service is now implemented as `lagiAzelLoadFieldMapResources`.
Read-only MAIN 06016E64..06016F0C confirms SCB sizes strictly greater than
4000 select VDP2 69000 (otherwise 6C000). PNB sizes at most 800 first fill
800 bytes with big-endian 0238 words; then the PNB loads into the caller's
pattern storage. The service preserves file order and native thresholds, adds
bounds/missing-file checks, and returns the selected graphics VRAM offset.
Failure does not promise rollback: the future task must restore backed-up
graphics if a load fails after mutation. No constructor is enabled yet.

Host execution tests the actual loader body with file/VRAM mocks across 20
threshold/fill combinations plus invalid capacity, missing graphics, oversized
graphics and failures of each load. Service Vita syntax passes. This does not
verify packaged asset lookup, rendered maps, task lifetime or device input.

Restore the actual setup/availability/context, resource loading and display,
cursor task, all native mode-specific input functions (`06017762`, `06017882`,
`06017964`), and return lifecycle before enabling the constructor. Preserve
both authored world-map and field-map choices and their native availability.
Use real SCB/PNB data, not an approximate flight-radar substitute. Device gates
are unowned/owned entry, current-area image, switching/navigation, cancel,
reentry and save/reload ownership. Source/host tests do not establish them.

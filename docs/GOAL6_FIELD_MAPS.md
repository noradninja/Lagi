# Goal 6: field-map lifecycle

Status: active; no hardware acceptance or runtime correction claimed yet.

Required acceptance: first and second map pickups, visible menu entry and
selection, native blue visited-area markings, normal cancellation, and the
same ownership/exploration after saving and reloading. Preserve accepted
rendering and performance. Do not modify extern/Azel.

## Current source audit

`lagi_map_menu_vita.cpp` falls back to a placeholder when parent availability
cannot be resolved or its enabled world destination has no backend. The
placeholder displays a selection message but never creates a field viewer.
This is a candidate for the invisible menu, not a confirmed reproduction.

Before this goal's binding correction, `lagi_map_parent_source_vita.cpp`
rejected nonzero authored field routes while gate 0024B89C was unresolved.
The module-manager
generator still leaves entry 5 (CHANGE.PRG) null; entry 6 (WORLD.PRG boundary)
does not substitute for it. Enabling the world slot without its native owner
and actual backend would not be a correct field-map fix.

Exploration reads use COMMON20A388[field], subfield record, signed rectangle,
and the signed word at record+8; native MAIN0601249C confirms the offset.
They query the existing getBit566 state, not a separate renderer cache.
The shared producer of explored cells still needs tracing. A3 exit bits alone
are not evidence that all map cells are marked.

## Newly established native gate producer

Read-only Disc 1 MAIN inspection identifies **06012434** as the setter:

- 06012434 loads the literal at 06012530: 0024B89C.
- 06012436 returns with delay-slot MOV.B R4,@R3 at 06012438.
- MAIN06011BB0 loads setter pointer 06012434 from literal 06011C30.
- 06011BB2 calls it with delay-slot R4=1 at 06011BB4.
- This occurs in initialization beginning 06011B90, immediately after the
  new subfield pointer is stored at field+8 and subfield+370 is cleared.

An aligned MAIN scan found the setter pointer at 06011C30 and no direct BSR
to the setter. This is not a whole-disc caller proof.

The source counterpart is `s_FieldSubTaskWorkArea::Init` in
mainMenuDebugTasks.cpp: identical field+8 store, subfield+370 clear,
`setFieldSubTaskVar0(1)`, field8/12 allocator exception, C210 fade endpoints,
overlay callback and state4 transition. The setter stores its argument to
the existing `fieldSubTaskVar0` byte. The source resolver now reads that byte
and passes the authored route into native availability instead of rejecting
all nonzero routes. No new flag, inventory dependency or forced value.

Production source/dispatch host tests pass, including gate0/1/80, nonzero and
FFFFFFFF routes, both field tables and malformed COMMON. Full battle Vita
VPK build passes. The world backend guard deliberately remains: entry5 is
still absent, so this correction alone does not claim to repair invisible
menu entry or enable world travel. Hardware validation is outstanding.

Evidence: original AZEL_1.bin MAIN LBA58, 253952 bytes, base06006000;
`disc_sh2_inspect` ranges 06011B90..06011C40 and 06012434..06012554.

## Outstanding

## Restored shared exploration producer

MAIN06012674 is the source `fieldTerrainCallback` stub called by
updateDragonSavedAnglesAndTerrain. The guarded generated replacement uses
the existing current-field/subfield COMMON20A388 hierarchy, rectangle at
record+0 and bit base at record+8. X divides by 00200000 and decrements when
the original X is negative; Y divides wrapped NEG(Z) by the same cell size
and decrements when original Z is positive. Exact boundaries retain these
explicit adjustments, rather than substituting floating point floor.

Native helper060125E8 bounds the rectangle and writes only flag IDs in
[095A,10D8), using the existing setBit566/save-state storage. No new visited
cache, per-frame allocation, rendering pass or GPU work is introduced.
The existing caller still owns special-field coordinate transforms.

Production-include tests pass 144 signed coordinate pairs (including exact
cell boundaries and INT32 extremes), flag-range endpoints, repeated marking
and malformed/null COMMON pointers. Full battle Vita VPK build passes.
Actual blue markings and save/reload still require hardware acceptance;
prior saves cannot retroactively gain cells never recorded by the stub.

The user clarified the invisible page follows **any** subsequent map pickup,
not a specific second location. Continue auditing shared acquisition/menu
state and resource lifetimes; do not add a pickup-specific workaround.

## Remaining acceptance and integration

### Correct native destination guard

Further trace found the backend guard itself checked the wrong entry.
`lagiAzelCommitMapWorldConfirmation` restores MAIN0601798C's
setNextGameStatus(4F); moduleManager's status4F row is mode6/parameter0.
Mode5 is CHANGE.PRG and is not that destination. The guard now checks6,
which has the existing WORLD.PRG boundary, instead of5. It does not alter
authored availability, disable a world slot or redirect travel elsewhere.
This removes a concrete cause of falling back to the nonfunctional page
when a valid world route exists. Confirmation can still reach the existing
non-playable WORLD boundary: **world display/travel remain unported**.

Source/dispatch tests distinguish slot5-only (reject) from slot6-only
(accept). Parent and resource ownership tests pass; the resource fixture
also reopens four maps with alternating large/small SCB destinations on
the same owner. Full battle Vita VPK build passes. Hardware reproduction is still needed to establish whether
this fully resolves the reported subsequent-pickup failure.

Save audit: loadSavegameScreen.cpp saveData copies the complete
mainGameState into sSaveDataRaw.mC_mainGameState (line117); readSave restores
that same member (line79). The callback writes its existing bitField through
setBit566, so no additional map sidecar or save-layout change is needed.
This is source evidence of inclusion, not on-device round-trip acceptance.

### Current-tree regression and save round trip

The context, view-task, transition, pickup-lifecycle, exploration, parent
source/dispatch, parent-task and resource-owner fixtures all pass together.
The generated production save/read fixture now includes all ten distinct
Disc1 COMMON20A650 map-item IDs (B3..B8, BA..BD) and a sparse pattern across
the entire [095A,10D8) exploration range. Ownership and both marked/unmarked
cells survive the temporary-file save/read round trip and legacy growth
migration; corrupt-file rejection preserves the existing live state.
The file transport is a host shim, not the Vita filesystem. No save-format
change was made and no device persistence or visual acceptance is claimed.

Next device acceptance on d93b9e1 or later: open/close the first map, acquire
another map, open/close and reopen the map page, verify newly travelled cells
are blue, save and reload, then verify ownership and blue markings remain.
Check input/cancel behavior and rendering/performance along the same route.
Do not test unfinished world travel as though it were part of this fix.

Trace map pickup state, parent/backend coupling and resource lifetimes;
reproduce the second-map path with fresh diagnostics; implement narrow
source-faithful corrections and tests; build Vita; obtain pickup/menu/save
and reload validation on hardware. Host tests alone cannot close this goal.

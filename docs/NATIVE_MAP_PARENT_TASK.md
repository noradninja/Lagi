# Native Map parent task integration

`lagi_map_parent_vita.cpp` assembles MAIN060179CE Init,06017A68 Draw and
06017B74 Delete around the restored native Map services. Its constructor
requires an explicit `LagiAzelMapParentSource`: verified availability,
field context (when present), and the original standalone/menu distinction.
It does not supply an invented field-route flag or infer availability from
inventory ownership.

Implemented parent flow:

- Normal opening starts layer0 at(-10,0) and layer1 at(0,-16), each for16
  authored Draw steps. After16 wait ticks, selection setup occurs on the next
  Draw. The increments are decimal10/16, not hex10/16.
- Standalone entry saves native cadence, requests one VBlank and starts at
  phase1. An available field map opens directly, as MAIN06017AA2..17AB8 does.
- Init applies authored highlights/absent-world cover, loads the original
  descriptor resources and creates a hidden native selection cursor.
- Selection preserves the pressed input word, B-before-confirm precedence,
  absent-slot skipping, rejection sound5, movement sound10 and highlight
  palette calls. Cursor/message state is shared with the actual child service.
- Mode1 hides the cursor, copies owned context/patterns to the native field
  view child and clears the prompt. A finished viewer returns to selection,
  or initiates standalone closing. Completion is observed at the task flag,
  not delayed until Delete.
- Mode2 starts the original horizontal confirmation. When Delete publishes
  its result, accepted YES uses the restored native status4F/saved-fade/menu
  exit service once; the parent then returns to selection. NO returns without
  requesting a transition.
- Normal closing clears the native text rectangle and starts reverse layer
  scrolling for16 steps. Setup falls through to the first wait decrement,
  leaving15 ticks. Standalone closing resets both live fade channels to black
  and finishes immediately. Only standalone Delete restores saved cadence.
- Parent-owned resources restore at parent Delete, after native task-tree
  child teardown. No raw child task pointer is retained past deletion.

## Live boundary and remaining work

`createMapTask` now selects the assembled parent when its native availability
is provable and every enabled transition has a registered overlay. The live
source resolver preserves the actual menu-id distinction: id1 is normal,
other ids are standalone. Supported proof cases are:

- Scene2 uses the already verified native town/current-return comparison.
- Scene0 before story bit193 never consults the field-route gate in native
  availability, so field context determines the field slot normally.
- Scene0 after bit193 can be resolved when its authored route is zero:
  both gate=0 and gate=1 then return zero. This is equivalence of native
  results, not assignment of a guessed gate. Field1's direct table and the
  other fields' pointer table retain their original lookup rules.
- Other native scene classes do not enable a world route; the native
  availability model and context resolver still decide their slots.

A nonzero authored field route with bit193 set remains unresolved because
0024B89C can change its result. Malformed required state also rejects native
dispatch. In those cases the existing placeholder is retained with an
explicit `[MapParent] native=0` log; no slot is silently changed to0/-1.

World travel has a separate real runtime limitation: the current trimmed
module-manager table leaves `overlayDispatchTable[5]` null. The dispatch
adapter reads that actual entry and retains the placeholder if an enabled
world choice would enter a missing overlay. It does not rewrite the authored
availability or pass world confirmation into a field viewer. Registering a
complete world overlay and resolving the separate field flag remain necessary
for full coverage. The optional native mode1 debug-grid flag/overlay and
hardware validation also remain. This is **partial live integration**, not
completion of the Map menu or broader menu goal.

The parent has no substitute map renderer, title strings, navigation solver,
world-transition destination or animation multiplier. Azel's existing
resource, view, cursor, message, sound and fade services remain the producers.

## Validation

`tests/map_parent_task_test.ps1` compiles the actual parent body with host
service doubles. It exercises normal and standalone lifecycles, sixteen-step
timing/fallthrough, field return, world result consumption, B precedence,
native scroll parameters/sounds/cadence, absent slots, copied input and
resource/status lifetime, resource failure, setup failure and cursor failure.
The constructor rejects invalid selection, absent selected world slot and
field-availability/context mismatches. Parent Vita syntax and a full local
VPK compile/link/package build pass, including a second incremental build
after the final constructor guard change. A further full build with partial
live dispatch also compiles, links and packages successfully. The three
parent/view/transition host suites and the new source/dispatch suite pass.
Package success and host service doubles do not prove real VDP1/VDP2
presentation, device input or successful on-device Map entry.

The resulting unstripped ELF retains `lagiAzelCreateMapParent`,
`lagiAzelResolveMapParentSource`, `lagiAzelCreateFieldMapView` and
`overlayDispatchTable` (verified with Vita nm), so these are no longer just
discarded unhooked sections. The local VPK SHA256 for this source state is
`4A5E9F8963AC43D044C305E6E06F80C62DCC43FEB5DCD8B64781D7FE9B19CE56`.
No hardware execution is claimed by symbol retention.

```powershell
./tests/map_parent_task_test.ps1
```

Run after entering the Visual Studio developer environment used by the other
host tests. `tests/map_parent_source_test.ps1` additionally executes the actual
live resolver and dispatcher with controlled native state: story short circuit,
both route-table forms, zero/nonzero results, context/town binding, malformed
state, source reset on failure, and null/populated real overlay-table entries.
This integration changes Map entry in the proven cases; it does not change
the cached-staging performance path or native animation counters.

For the next hardware Map check, open Map from the main status menu, confirm
the field choice, move/pan, cancel back to selection, then cancel to status.
Retain the log and screenshots. `[MapParent] native=1` identifies the selected
parent route, not a visual PASS; `native=0` identifies the retained placeholder.
Record its `(field,slots,selected)` state so an unresolved route is not mistaken
for a failed field viewer. World travel is not a valid test target while the
actual overlay entry is null. Existing FPS tests on the staging build remain
valid; a latest build adds the Map dispatch change described here.

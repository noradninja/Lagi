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

This task is compiled but **not yet selected by `createMapTask`**. The current
placeholder entry is still active. The native field-route byte0024B89C
remains unresolved; the explicit constructor boundary prevents fabricating
it in order to claim a complete menu. The already bound town availability
can supply a valid source separately, but no town-only replacement is being
presented as completion of the field menu. Live dispatch, the original
mode1 optional debug-grid flag/overlay, and hardware validation remain.

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
after the final constructor guard change. The three parent/view/transition
host suites pass together. Because live dispatch is not hooked up and the
link uses section garbage collection, package success does not prove this
factory is retained or exercised by the running game. Host service
doubles do not prove real VDP1/VDP2 presentation, device input or dispatch.

```powershell
./tests/map_parent_task_test.ps1
```

Run after entering the Visual Studio developer environment used by the other
host tests. The performance-test path remains unchanged by this unhooked task.

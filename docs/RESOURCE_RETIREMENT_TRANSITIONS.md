# Resource retirement across overlay transitions

## Observed failure and bounded conclusion

The 2026-10-19 Arachnoth victory log reaches E014 and loads its sound bank.
The matching fatal caller relocates to `std::__glibcxx_assert_fail`, rather than
the earlier `loadTownPrg` assertion. The partial core has no useful caller stack,
so the exact observed bounds check cannot be established from it alone.

Source inspection finds a reproducible lifecycle failure consistent with that
assertion: `initDramAllocator` replaces the global resource-entry vector, while
finished NPC resource tasks can receive their deferred Delete later. Arachnoth
owns slot 8, but E014 has only four entries. The old Delete reads entry 8 before
checking whether its task still owns that entry. The production-body test
reproduces this transition using a checked table, not an assertion bypass.

## Changes

Generated `kernel/graphicalObject.cpp` checks both bounds and pointer ownership
before detaching an entry. Old payload cleanup still executes; a stale task
cannot clear a new task's refcount/pointer even when its old index is reused.

The corresponding VDP1 free path previously always inserted a node into the
current allocator and cleared its VRAM range. That is unsafe for old overlay
nodes. It now locates the node's owner in the active allocator chain BEFORE
dereferencing the node. Active nodes return to their owning allocator; retired
nodes cannot clear replacement-scene VRAM or be spliced into its free list.
The allocator's embedded full-pool node remains supported. This changes only
service-side resource lifetime handling, not Azel's encounter/script decisions.

Battle builds additionally wrap the C++ assertion reporter. Before forwarding
the original failure, the synchronous fatal log records bounded file/function/
condition strings, source line, and immediate caller. E014 startup has explicit
stage markers; the historical log field name `lastE011Stage` is retained for
compatibility, but its value can now begin `E014:`. Assertions remain fatal.

## Validation

`resource_retirement_test.ps1` executes actual generated Delete and VDP1 free
bodies for empty/shorter/reused tables, normal ownership, nested active pools,
retired pools and an unowned pointer. It checks that stale frees do not clear
VRAM or invalidate textures; payload deallocation is mocked. `fatal_trace_test`
also exercises the C++ assertion details and unchanged failure forwarding.
Battle and battle-disabled Vita packages compile. Hardware passage through
victory/E014 is still required before declaring the observed crash resolved.

## Remaining work

Charge placement/recovery, cinematic PCM, boss slime/Prism effects, HUD flicker
and GPU time remain open. The original Charge common tail's negative `0x28F`
coefficient is present on disc (`0605A128`); do not change its sign merely because
it looks suspicious in reconstructed C++. No arbitrary relocation, animation
slowdown, or depth-test exemption is part of this change.

## Follow-up run: world-grid assertion identified

Run `1792436777` now reports the C++ condition and container type explicitly:
`__n < this->size()`, `std::vector<sCellObjectListNode*>::operator[]`, after
`E014: startCameraTask(pThis)`. This identifies the cell-object list, not the
resource-entry vector; the earlier resource-retirement bug was real but did not
resolve this remaining crash. The partial dump still does not provide a complete
stack, so avoid claiming its missing caller chain was recovered.

`initTownGrid` cleared the object list and dimensions but only rows 0 and 1 of
the **8x8** cell-task ring. A battle can populate any ring row. E014 installs
town callbacks, then `initWorldGridData` calls `resetWorldGrid` before creating
its new cell-object list. Surviving battle ring entries therefore reached town
`deleteGridRow` / `deleteCellObjects` with no corresponding list entries.

The generated adapter now clears every row at the existing initialization
boundary. It does not bounds-skip live objects, invent dimensions, retain old
scene objects or suppress the assertion. The existing parent task teardown
continues to own outgoing battle tasks.

`town_grid_reset_test` executes actual generated init/delete-row/delete-object
bodies with the native grid drain. It populates all 64 ring slots, switches to
an empty list, drains safely, and checks normal live-object cleanup separately.
The battle and battle-disabled fixtures and Vita builds pass. Actual E014
playback remains a hardware acceptance gate.

The same run confirms the audio service loads bank **67**, `BOSS01_2.SEQ` /
`A3BOSS.SEQ` / `A3BOSS.BIN`, and starts Charge sound 118 on **control 6**, data
bank 3 / sound 17. This confirms routing, not subjective listening acceptance.

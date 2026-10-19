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

# Boot presentation transitions

The CPU loading scanout stays visible while Neptune initializes and prepares
the immutable disc-selector textures. The ready selector crossfades from a
snapshot of that loading frame over 30 VBlanks using a premultiplied GPU overlay.
Steady selection continues to use immutable textures, without CPU composition.

After selection, the existing 30-VBlank fade reaches black. The black GXM
scanout retains display ownership during native initialization, until Azel
presents the opening Cinepak. It must not return to the CPU loading buffer.

`tests/launcher_transition_test.ps1` checks source contracts and opacity
endpoints; it is not a device visual test. Validate on Vita: loading remains
visible during preparation, crossfades to the selector, then fades to black
without loading text or stale pixels before the opening movie.

## Startup regression triage

The fa5b608 device log reports `NBG0 OVER PROGRAM` creation failure before
renderer readiness. Thus neither GPU selector entry nor native movie display
can run; the CPU fallback leaves Loading visible while native tasks advance.
The expanded NBG shader binary is 17,684 bytes and has multiple resident
blend/MSAA variants, in addition to the other VDP1/VDP2 programs. The original
64 KiB fragment-USSE pool is a likely capacity constraint, not a proven error
code (the earlier log omitted that code).

The patcher now reserves 512 KiB fragment USSE and 256 KiB general buffer
storage, with the vertex pool unchanged. GxmPatcher records actual occupancy
at readiness and the numeric error/occupancy if NBG0 over creation fails.
These are fixed startup allocations; no per-frame work is added. Device
acceptance requires renderer readiness, DiscSelectorGXM ready, visible
crossfade, and native playback without the Loading overlay.

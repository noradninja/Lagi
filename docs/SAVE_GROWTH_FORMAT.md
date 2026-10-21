# Growth-state save extension

The generated Vita common header appends 48 bytes to the retained game stats:
five current growth counters, five cumulative counters, and eight proficiency
bytes (four used, matching native indices 1..4). Existing stats fields retain
their offsets. Both battle-enabled and normal builds share this layout.

New saves use version `0x10001`. The platform validates a `0x10000` save before
inserting zero growth history in memory, shifting the remaining battle-results
and button-config bytes intact, and recomputing version/checksum. Reading does
not rewrite the slot. The first subsequent write retains the legacy slot as
`PANDRA_3_0N.v10000` alongside the new save. This retained copy is not overwritten
by subsequent saves. If retaining it fails, the upgrade write is refused.
Older builds cannot read `0x10001`; use the retained legacy file for rollback.

Historic growth preferences cannot be reconstructed: previous builds omitted
these counters. Migration therefore preserves recorded data and starts only
the missing history at zero. No fixed game-stat offsets are cast into host
objects, and no pinned Azel files or game assets are changed.

`save_growth_migration_test.cpp` covers legacy byte preservation, new-version
roundtrip and corrupt/truncated rejection. Real Vita load/save and rollback
acceptance remain required. Keep an independent backup before testing.

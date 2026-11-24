# Green Oasis hardware run - 23 November 2026, 16:20:44

Source checkout inspected: ee1c186, diagnostics/garil-cliff-current.
Capture directory: D:/Users/Noradninja/Videos/Lagi/2026-11-23 16-20-44 - Lagi - Lagi.
Supplied symbol file: E:/dev/Lagi/cmake-build-vita-baseline/lagi.velf.

## User-confirmed working

- Ruin activation interaction.
- Baby worm rescue, Secret Worm Family Passage unlock, entry and return.
- Mauler gun modification attachment.

These confirmations do not establish completion of the separate ruin-destruction
sequence. The boulder-shaped objects still cannot be targeted. Preserve the
required authored sequence: activate, destroy required objects, destruction
cinematic, exposed entrance. Do not force completion flags.

## Open hardware failures

- Initial ruin cutscene camera/composition is wrong. The first seconds of the
  video show a sloping floor boundary while the distant cliff band stays level.
  The dialogue includes finding the ruin that activates this one.
- Battle environment geometry/sprites appear misplaced. Supplied screenshots
  show Stryder Pack; do not assign these images to a formation solely by name.
- Sand whirlpools, including beneath the baby worm, are missing.
- Oasis small tree sprites appear incomplete.
- Final combat entry crashes under the white loading cover.

## Crash evidence and limits

The final startup is BTL_A5.PRG, overlay 1, sub-battle 14 (log line 33262).
The native BTL_A5 battle record at 060AC9F8 + 14*20 selects formation table
060ABE9C (constructor 060547DA) and environment callback 0605A72E. Both are
already dispatched by the generated adapter. Native callback 0605A72E uses
Init 0605A116 and the 2x2 grid 060AEDA0. This is not evidence of an
unsupported Pludger formation.

The supplied partial gzip dump is 3,072 bytes. Incremental decoding recovers
8,951 ELF bytes and does not reach gzip EOF. Thread and synchronization notes
survive; readelf reports later notes beyond EOF, including the fault-context
portion. No fault PC, symbolized crashing function, or cause is established.
Do not infer these from thread-info bytes. Keep the original dump unchanged.
The supplied VELF has not yet been matched to a surviving module/build record.

Rejected source hypothesis: the variant adapter does NOT omit model-count
initialization. lagiCreateBattleEnemyModels sets count by reference and resizes
the model vector before indexing. No change was made for that hypothesis.

## Shared entry diagnostics checkpoint

CMake generated adapters now bracket reset3dEngine, camera reset, initTownGrid,
overlay initialization, engine sub-initializers, dragon creation and environment
dispatch with BattleEntryPhase begin/end messages. These are transition-only,
shared across battle overlays, and do not alter progression or formation logic.
Generated replacement boundaries are checked during configuration.

Validation: build-vita configure, compile, VELF/SELF conversion and VPK packaging
passed. This is a diagnostic checkpoint, not a crash fix or hardware acceptance.
No additional capture has been requested; continue tracing existing evidence.

## Further source findings to investigate

- Oasis cell-object table 0608FFA8 contains 112 placements, all three registered
  sand-pit factories (44 at 0605ED66, 42 at 0605EDBA, 26 at 0605EE34).
  No fourth boulder factory exists in that table.
- Native environment update callbacks 0605A2C4 and 0605A2C8 are empty RTS/NOP;
  using a null Update in the additional environment adapter is not the cause.
- The shared battle environment cell renderer in BTL_A3_map6.cpp still has an
  unimplemented billboard list at cell+10 and dynamic-object creation at +14.
  Check whether the affected Garil cells contain such records before restoring
  their original behavior. Do not guess scenery offsets.

## Third scenery grid restoration

The original Oasis environment descriptor has a third grid at 0609146C. Its
central cell contains 64 records: models B30 (5), B38 (15), B34 (14), ADC/AE0/AE4
(4 each), AE8/AEC/AF0 (6 each). All were omitted by readGrid3 returning null.
Native field draw 060780A2..06078144 traverses 16-byte records, clips and checks
LOD, then calls common 0601E424. That primitive keeps view translation but sets
basis diag(-1,1,1), unlike second-list billboard 0601E356. Optional collision
uses matrix helper 06077010 and shared collision producer 060771C0.

The generated shared field adapter now decodes the third list using the existing
16-byte decoder, owns one active layout's cells, and submits that exact basis
through the normal model bridge with a dynamic view-relative scope. Replacing
the active layout replaces the decoded storage; unrelated layouts are guarded.
No extern/Azel edits or guessed placement/scale/culling values were introduced.

Validation: production-helper host test checks the 64 native records and native
instruction anchors, clipping, LOD, reflected identity basis, translation,
collision dispatch, layout guards and matrix-stack restoration. Vita configure,
compile and package passed. Restored visual coverage awaits hardware acceptance;
this does not yet establish that every missing tree or whirlpool is fixed.

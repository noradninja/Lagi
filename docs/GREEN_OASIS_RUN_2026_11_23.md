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

## Whirlpool placement selection correction

Native selector 0605ED0C returns one for counter modulo interval == zero
(0605ED54..ED5C); factory 0605ED66 creates only when that return is nonzero
(0605ED72..ED74). The adapter had inverted this branch. It now skips nonzero
remainders and creates the original selected placements. Authored intervals at
0609C678 are 5,4,3,2,1,1 for thresholds 10,20,30,40,50,99999. At interval one
the old code suppressed every pit. No progress bits or thresholds were changed.

Validation: native branch evidence plus compiled production condition across
112 intervals and 224 placements each passed; Vita compile/package passed.
Visible whirlpools and progression still await hardware acceptance.

## Ruin discovery camera format correction

Native callback 0605A26C dispatches 0607272E, the orbit camera, with data
0608CFB4 and anchor 06099FD4. The old adapter decoded that data as 60-byte
position-camera records and dispatched cutsceneTaskInitSub2. The authored data
instead contains two 36-byte orbit records (185 and 200 frames), then a zero
terminator. Treating their angles/radius as world positions explains invalid
framing without requiring guessed camera offsets.

Discovery now owns/decodes those orbit records and uses existing shared
cutsceneTaskInitSub3 with its decoded owned anchor. Other position-camera calls,
including monolith destruction, retain their authored format. The shared orbit
Update also now integrates radius by m48 before evaluating position, as native
060723A8..B6 does; the reconstruction had omitted that step. The second discovery
record moves radius from A0000 to 78000.

Validation: compiled production decoder/service fixture checks original records,
owned storage on repeated invocation and generated discovery routing. Original
radius instructions and generated update ordering are checked. Vita compile and
package passed. Camera framing still requires hardware acceptance.

## Remaining investigation

Boulder targeting/connected ruin destruction, battle scenery placement and the
combat-entry crash remain open. The particle-named subfield-4 entity is not a
small cosmetic effect: native 0605E99C initializes models, per-model animation
buffers and a controller, while upstream Init/Update/Draw remain stubs. Trace
its relationships to the required ruin objects before implementing any targets
or completion flags. No invented boulder count or story gate was added.

## Battle scenery column-streaming restoration

Native initGridForBattle 06014432 registers column creation 0601489E and column
retirement 06014A1C alongside row callbacks. The shared reconstructed column
callbacks were empty. They now stream five cells (-2..2), wrap the authored grid
indices, apply signed repeated-world X/Z translations, populate the same 8x8
ring and retire/clear outgoing cell tasks. They use the existing battle cell
Init/Draw and object factory boundary; no area-specific mesh offsets or floor
height changes were made. The separate dynamic-cell-object factory remains
unimplemented and is not claimed restored here.

Validation: production adapter exercised 722 column creations/retirements across
negative and positive world coordinates, verifying resulting positions, ring
slots, untouched neighboring slots and repeat retirement. Native instruction
anchors from the original common RAM snapshot were checked. Vita compilation
and packaging passed. This fixes a confirmed streaming omission, but does not
establish that the reported apparent battle floor-height mismatch is resolved.
The user's current test of 4ee81e8 predates this streaming change.

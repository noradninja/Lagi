# A3 waterfall and water-layer investigation

## Typed waterfall emission positions

The six native waterfall emission helpers used Saturn task byte offsets (+8
for the data pointer, +C for the small waterfall frame counter) on polymorphic
C++ work areas. Those offsets do not describe the host/Vita task layout.
The generated-source adapter now infers the concrete task type and accesses
`m8` / `mC_frameCounter`. The pinned Azel source is unchanged. Native positions,
velocities, quad resources, RNG ordering, visibility gates and audio are retained.

`tests/waterfall_position_test.ps1` extracts the generated helpers and exercises
all six with deliberately non-native C++ layouts, checking the emitted positions,
frame-counter lookup and 26 RNG draws. The host fixture and fullres Vita VPKs
(battle enabled and disabled, stochastic filtering and lookup enabled) pass.
This is not on-device visual validation.

## Native pool billboard projection

The native A3 06078A2A path calls MAIN 0602E5CC with scale (+18) and sorting
bias (+1C). Pool mode 1 previously called the unscaled simple particle service.
It now uses both native arguments: authored frame origin/dimensions are scaled
before projection, while only the sorting key uses clamped view-Z plus bias.
The key retains the SH-2 low-32-bit multiply followed by a 16-bit shift. GPU
depth and projection still use the actual position. Mode 0 and other particle
callers retain their existing paths; native resource ownership is unchanged.

`tests/particle_origin_service_test.ps1` executes the production projection
with small/large negative and positive sorting biases, including both clamp
boundaries, and checks unchanged geometry/world depth, scale, source and LUT.
Both fullres Vita configurations build. Splash appearance still needs hardware
comparison against the supplied Saturn reference.

## Still open

## Native emission-rule audit after hardware feedback

The previous typed-position fix retained reconstructed Azel helper logic;
that did not establish that the reconstruction's emission gates were faithful.
Hardware feedback prompted a comparison with the original FLD_A3 instructions:

- Type 0, 0605BFA0: both families each visible update, unchanged.
- Type 1, 0605C088..0605C09C: one RNG bit chooses droplet OR mist. The C++
  reconstruction incorrectly emitted both, without the selection draw.
- Type 2: factory 0605C21C compares model-table pointer with 06091CF8 and
  installs update 0605C18E. That update has no falling-family emission;
  bit 0 gates a splash with spread 80000.
- Types 3/4 retain update 0605C12A: periodic falling family on frame & 7 == 0,
  then low two RNG bits == 0 gate a splash with spread 40000.

The adapter now preserves those factory-specific rules, independently of model
appearance or scene location. The fixture exercises both outcomes of the small
variant and large variant gates, family identity, RNG consumption and spread.
No new effect positions, water planes, or renderer-side waterfall detection
are supplied by Lagi. Native model tables and runtime quad resources remain
Azel-owned. Both Vita configurations compile; visual acceptance is pending.

## Layered water remains open

The Saturn reference also shows layered water coverage absent from Lagi.
Collision geometry and quad-debug screenshots alone do not identify whether the
missing visible contribution is model submission, VDP1 material processing or
VDP2 rotation-plane composition. No guessed water planes or blanket transparency
override have been introduced.

### Disc evidence

`tools/audit_a3_water_materials.ps1 -Disc <MODE1/2352 bin>` reads ISO extents,
the three native A3 outdoor grid tables and their visible LOD model resources.
It writes no extracted game data. `-ShowHorizontal` additionally lists local
constant-Y quads (not a claim that these are water).

On the supplied US Disc 1, subfields 0/1/2 reference 68/47/95 distinct visible
model slots and 1415/1096/1772 quads respectively. None of those quads has
CMDPMOD low color-calculation bits selecting half-transparency. These counts
exclude billboard-grid and task-owned models; this is not an exhaustive proof
of the scene's material use.

Subfield 2 has a third grid at 0608BCE4, currently unported by `readGrid3`.
The original FLD_A3 0606FF48..0606FFB0 loop walks its 16-byte records,
clips/translates them and invokes the collision helper 0606F5CC using the
descriptor's second model slot. It does not submit the first slot as a visible
model in this normal draw loop. Thus this missing reader cannot simply be
treated as the missing visible water planes.

Neptune currently resolves RBG0 and draws it before world geometry. Native
sprite priority/color-calculation versus that water layer still needs a
source-faithful composition trace. The supplied screenshots establish a
visible discrepancy, not yet a proven isolated cause or a tested floor fix.

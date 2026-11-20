# Native target selection and indicator visibility

Goal 7 follow-up to the completed-image UI-priority correction (46774df).
No extern/Azel edits; hardware angle-dependent visibility is not accepted.

## Authored quadrant eligibility already executes

Disc1 BTL_A3.PRG LBA258144, 385044 bytes, base06054000:
target-system update0605CFAC uses the dragon's current quadrant and each
target's authored mask80000000/40000000/20000000/10000000. It resets active
targets' selectable index, filters gun eligibility, sorts the native pointer
array and assigns one-based indices only to selectable entries. The generated
production source retains this gate; it is not replaced by an all-visible
Lagi targeting adapter.

The actual production update and eligibility functions now execute in a host
fixture covering2048 quadrant/state combinations, laser-only bit2, disabled
100000, non-enemy1 and retired40000, plus repeated quadrant changes and
selected-pointer/index ownership. Task transforms, distance and projection
services are mocked; this proves these decisions, not Vita world positions.

Native weak-marker update060695F0..0606970A gates on selected-target bit200000
and disabled/retired state, not directly on quadrant. The gun-selection engine
owns200000. Normal cursor update additionally uses the selectable indexm5A.
Native draws06069094/0606970C pass target+4 into visibility06068BD0, matching
the reconstruction; blindly changing that pointer to target+8 or adding a
marker-only facing rule would not be a source-faithful correction.

## Confirmed debug-projection defect corrected

Gun eligibility0605C75E..0605C8EA normally returns true after native flags and
altitude checks when debug flag1F is clear. With flag1F set it checks the
original position's near/far range and projected viewport. Native0605C8AE..
0605C8DC requires left<=X<=right and bottom<=Y<=top. getVdp1ScreenResolution
returns left/top/right/bottom. The reconstruction reversed both Y comparisons,
rejecting every ordinary interior point for the full debug-projection path.

A guarded generated-source adapter restores just those comparisons. The
production fixture failed on the previous code's interior viewport test before
the change. Coverage includes203x203=41209 viewport samples, all four
boundaries and just-outside samples, near/far equality and outside values, plus
the native non-debug projection bypass. No normal-mode camera/facing behavior
is changed. This is not claimed as the cause of the user's all-angle report.

## Open validation

Retest weak-marker overlap on46774df or later, and capture ordinary selection
while moving between quadrants. If the wrong-side issue remains, trace the
live authored hotspot flags, selectable indices, target positions and selected
flag lifecycle rather than inventing an extra visibility filter. Accepted
ordering, normal combat performance and Goals1-6 remain regression gates.

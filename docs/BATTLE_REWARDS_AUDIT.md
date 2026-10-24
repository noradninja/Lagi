# Combat rewards and progression audit

Goal 2 is active. Current combat performance and loading behavior are accepted;
do not equate that with reward/progression acceptance.

## Rank identity correction

Original US Disc 1 BTL_A3.PRG is at LBA 258144, length 385044, base 06054000.
At 0606EA60, MOV.W @(2,R0) reads byte offset 4 from the manager, not offset 2:
the SH2 word displacement is scaled by two. This is m4_previousBattleOverlayId.
At 0606EA88, MOV.W @(4,R0) reads offset 8, m8_previousSubBattleId. The table
literal at 0606EB60 is 060AB010; the manager getter at 0606EB5C is 0600675E.
The existing rank adapter incorrectly indexed with current/remapped IDs.

The shared adapter now uses the previous IDs as the native code does. No
thresholds, scoring bands or XP multipliers change. battle_rating_test.ps1
checks these original disc instructions/literals and executes the production
include. The fixture deliberately makes current IDs 99/99, different from
the previous IDs, while checking all 25 score combinations over 17 overlays
and ten subbattles, threshold equality, update division and null tables.
This is a bounded source/instruction check, not whole-battle Vita acceptance.

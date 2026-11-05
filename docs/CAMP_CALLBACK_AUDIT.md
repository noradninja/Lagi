# Camp callback audit — 2026-11-05

Camp callback coverage remains incomplete. The current registration and save
fixes are checkpoints within this audit.

Sixteen additional script callbacks are restored in this checkpoint, covering
dragon feeding/rewards, player animation banks, NPC turning/animation/flags,
random choice and the direct camera restore entry. Their source-level binding
checks and production fixtures pass. Two script targets and both NPC task
definitions below remain pending. NPC receivers must be implemented before the
connected script services can establish a playable scene.

## Native script inventory

Read-only inspection uses US Disc 1 TWN_CAMP.PRG at LBA239428, base06054000.
The provisional static walk starts at the distinct script roots referenced by
all nine setup records and adds object interaction scripts reached by native
object registration. It follows conditional branches, script calls, switch
entries and native-call operands across story alternatives. Current inventory
is55 distinct native function/arity pairs. Bound means a generated dispatch
entry exists. Semantic parity and hardware acceptance remain separate gates.
Opcode38 is a no-operand Yes/No dialog, recovered from MAIN600D660 and task
factory601C6CA. Its runtime handler is absent. Table-default handling and all later-game paths still need
independent native instruction validation before this becomes a completion gate.

| Native function | Arguments | Dispatch | Call sites | First call site |
|---|---|---|---|---|
| 0600CC78 | 2 | Bound | 1 | 0605807D |
| 0600CCB4 | 1 | Bound | 9 | 06065B80 |
| 0600CDD4 | 0 | Bound | 10 | 060580F9 |
| 06014DF2 | 1 | Bound | 90 | 06058140 |
| 06026DA4 | 0 | Bound | 1 | 06060487 |
| 06027110 | 1 | Bound | 2 | 0605A259 |
| 0602C2CA | 1 | Bound | 9 | 060591CD |
| 0602C32A | 1 | Bound | 8 | 06065C24 |
| 0603011E | 0 | Bound | 1 | 06060361 |
| 060541C4 | 1 | Bound | 9 | 06065B8C |
| 06056408 | 1 | Bound | 1 | 06060A3C |
| 0605641C | 1 | Bound | 1 | 06060A4C |
| 06056484 | 1 | Bound | 1 | 060580A4 |
| 060564B4 | 1 | Bound | 1 | 060580C4 |
| 060564D6 | 0 | Bound | 1 | 06060998 |
| 060564FE | 0 | Bound | 1 | 06060980 |
| 06056526 | 1 | Bound | 5 | 06060BE4 |
| 06056544 | 1 | Bound | 5 | 06060B70 |
| 06056562 | 1 | Bound | 1 | 06060A93 |
| 06056574 | 1 | Bound | 5 | 06060BA8 |
| 06056588 | 2 | Bound | 4 | 06060B2C |
| 0605664A | 1 | Bound | 2 | 06060A18 |
| 06056662 | 1 | Bound | 2 | 06060A28 |
| 06057B62 | 1 | Pending | 1 | 060604F0 |
| 06057B76 | 1 | Pending | 1 | 06060514 |
| 06057F46 | 0 | Bound | 1 | 06060AD8 |
| 0606AECC | 0 | Bound | 1 | 060580DA |
| 0606C70C | 1 | Bound | 9 | 06065BC8 |
| 0606C952 | 0 | Bound | 10 | 06058608 |
| 0606C95C | 0 | Bound | 10 | 0605861C |
| 0606CB54 | 0 | Bound | 1 | 0606EB74 |
| 0606CC0E | 0 | Bound | 3 | 060587BD |
| 0606D416 | 0 | Bound | 1 | 0606EB88 |
| 0606D542 | 1 | Bound | 38 | 060584B8 |
| 0606D580 | 0 | Bound | 1 | 0606EB9C |
| 0606D5BC | 0 | Bound | 3 | 060603B1 |
| 0606D5F2 | 0 | Bound | 13 | 06060431 |
| 0606D608 | 1 | Bound | 5 | 060609E0 |
| 0606D790 | 1 | Bound | 2 | 06065CEC |
| 06070198 | 3 | Bound | 13 | 06058498 |
| 06070268 | 2 | Bound | 5 | 060586F1 |
| 060704C4 | 4 | Bound | 9 | 06065B98 |
| 060704F2 | 4 | Bound | 9 | 06065BB0 |
| 06070560 | 3 | Bound | 80 | 060584C4 |
| 06070664 | 3 | Bound | 48 | 060584F8 |
| 0607091E | 1 | Bound | 1 | 0605C8EB |
| 06070932 | 1 | Bound | 1 | 0605CCC0 |
| 06070946 | 1 | Bound | 22 | 060584AC |
| 06071A78 | 0 | Bound | 4 | 06058FBD |
| 06071A90 | 0 | Bound | 5 | 06059320 |
| 06071ADE | 0 | Bound | 1 | 0606038C |
| 06071B40 | 2 | Bound | 9 | 06065C30 |
| 06071CA4 | 1 | Bound | 9 | 06065C10 |
| 06071DA8 | 1 | Bound | 1 | 06058094 |
| 06071E20 | 1 | Bound | 13 | 06057FC9 |

## Missing task dispatch

Four original object definitions have factories in Camp. Two additional
definitions reached through object registration remain absent.

| Definition | Init | Update | Draw | Delete | Native size |
|---|---|---|---|---|---|
| 06080148 | 0606F3F0 | 0606F530 | 0 | 0606FF04 | 17C |
| 0607B5BC | 06057714 | 06057A68 | 0 | 060579F0 | 1B4 |

These require recovery or verified reuse of matching task implementations.
Common NPC movement/animation callbacks must be checked against their concrete
receiver types and registration lifecycle, including Edge and the Camp dragon.

## Remaining acceptance

- Resolve every pending native call with correct argument interpretation.
- Recover both missing NPC task paths, including their child callbacks.
- Recover runtime opcode38 and validate static coverage against native dispatch.
- Exercise startup, dragon recovery/interactions, other characters, save station,
  save/load return state, normal exits and repeated Camp entry.
- Preserve the tested VPK, unstripped lagi and lagi.velf together.

WORLD sky mapping is confirmed fixed by the user. Camp remains unaccepted.

# Tutorial 1 movement lesson progression

The user's second selectable lesson stalls, while the last three immediately
complete. These are distinct native integration failures, not draw-order fixes.

## Movement lesson repair

Disc 1 BTL_T0.PRG, native function `06059044`:

| Controller phase | Native destination | Helper |
| --- | --- | --- |
| 1, 4, 0x13 | 0605938A (R10 = 0605A6E8) | Wait for inactive attack, create modal input |
| 9, 0x14 | 06059392 (R12 = 0605A72A) | Scroll/advance text |
| 2, 0xA, 0x15 | 0605939A (R8 = 0605A776) | Timed confirmation/close modal input |
| 6 | 06059094 -> 06059392 | Scroll/advance text |

The retained upstream switch instead sent phase 2 to scroll, phase 4 and 9
to timed confirmation, and phase 6 to modal creation. Scrolling phase 2 moves
to phase 3 without deleting the modal input child or setting
`m100_attackAnimationFinished`. Phase 3 waits for the still-active attack
sequence and cannot proceed. The adapter restores the original helper dispatch,
without changing the engine's input-lock rules or modifying extern/Azel.

`tests/tutorial_movement_test.ps1` compiles the actual generated phase function.
It checks every affected branch and a simulated 1 -> 2 -> 3 -> 4 -> 5 modal
handshake. Helpers/engine/task services are mocked: this is a routing regression
test, not proof of full engine execution or hardware tutorial completion.

Vita build including VELF/SELF/VPK succeeds. Existing font, panel and contextual
text fixtures pass. Device validation of this repair remains required.

## Separate missing lessons, not fixed by this commit

The original encounter table at `060A989C` contains:

| Lesson/sub-battle | Formation table | Callback |
| --- | --- | --- |
| 0 | 060A934C | 060540FC |
| 1 | 060A9334 | 060549B4 |
| 2 | 060A9364 | 060586E0 |
| 3 | 060A937C | 0605B00C |
| 4 | 060A9394 | 06055A54 |

Only the first two callbacks are currently dispatched by BTL_T0_data. The last
three are absent, so no formation keeps `m3B2_numBattleFormationRunning` alive;
the ordinary completion check ends the battle. Restore each original formation
and its lifecycle before adding dispatch. Do not fake an immortal formation,
reuse arbitrary enemies, or suppress completion globally.

The existing supplied log contains entries only for sub-battles 0 and 1; it does
not itself demonstrate execution of the last three lessons.

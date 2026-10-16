# Native Map world-route confirmation

The new compiled service restores the specialized horizontal YES/NO task used
by MAIN06017736. It is **not yet connected to the incomplete Map parent** and
does not enable world-route availability or change the game status itself.

Read-only Disc1 evidence:

- 06017736 calls0601C710 with x8,y39hex, a retained child output and result.
- 0601C710 allocates2C bytes and uses COMMON21006C: null Init,
 Update0601C4F0, Draw0601C5D8, Delete0601C6B0.
- The constructor starts selected0, cursor frame/ticks0, width10hex,height4.
- 0601C42C draws the blue box, sets interior text rectangle(x+2,y+1,w-4,h-2),
 resets text flags and cursor to rectangle origin, and renders the string
 pointer at COMMON210048. That resolves to2100CC: `  YES   NO`.
- Input right80 selects1/NO, left40 selects0/YES, without wrapping. Right has
 priority if both are pressed. Confirm6 wins over cancel1, returning
 selected XOR1 (YES1, NO0); B returns0. Move/open/confirm/cancel sounds2/3/0/1.
- Phase3 disables Draw, phase4 marks finished. At0601C5B6 R4 is workArea-24;
 the write atR4+C clears the native task-header Draw callback, not result+C.
- Draw uses the eight ten-byte cursor records at COMMON20FFE0, palette7F0,
 horizontal positions x+2 and x+8, and y+1. Animation adds current vsync and
 advances at ticks>=2, resetting ticks to0 and frame modulo8.
- Delete clears the interior text and full box, then clears the child output.

`lagiAzelCreateMapWorldConfirmation` owns a shared result/readiness/completion
object so the future parent can poll completion without dereferencing a freed
task. It validates COMMON/string bounds, uses native blue-box/text/cursor
services and biases the existing cursor packet helper to preserve the actual
current VDP1 local origin rather than assuming its hardcoded176/112 origin.
Init failures finish without drawing/erasing unrelated menu regions. COMMON
remains owned by the normal native runtime for this task's lifetime.

Host tests execute the actual production task/factory with mocked native
services: text rectangle and string, all input branches/priorities, no wrap,
sounds, result lifetime, delayed cursor disable/finish, both vsync cadences,
cursor frame wrap/local-origin offsets, cleanup and invalid COMMON. Vita
C++20 syntax passes. These checks do not prove full linking, rendered cursor,
font/layout, task-tree cleanup or device behavior.

The transition service `lagiAzelCommitMapWorldConfirmation` now restores
MAIN06017978's accepted-result path: set game status0x4F, reset both saved
gameplay fade channels through `m40E4->m2B0` when a backup exists, then set
the menu exit byte `m40AC.m2`. It does not touch the live menu fades.
The parent must call this once, after the confirmation task has finished;
unfinished, running and NO results are rejected. Production-body host tests
check all Boolean guard combinations, native call order, both target addresses,
and the null-backup path. Vita syntax passes; parent hookup and device
validation remain pending.

The assembled parent consumes this confirmation and transition service
(`NATIVE_MAP_PARENT_TASK.md`). Live dispatch now covers provable native
availability but retains the placeholder if an enabled world route would
enter the current null world-overlay slot5. Full live world travel still
requires that backend and the original nonzero field-route gate0024B89C.
Neither world travel nor on-device confirmation is claimed validated.
Do not replace the world route with another field viewer or pass39 as a fake
string pointer to the generic vertical `startDialogTask` constructor.

PowerShell host test, from the checkout after loading Visual Studio tools:

```powershell
./tests/map_confirmation_test.ps1
```

# Native menu animation cadence

## Cause and original state

The main status menu's native Init saves `vblankData.m14_numVsyncPerFrame`
in work-area byte1 and sets the live value to1. Gameplay normally uses2.
Native save/load and Dragon menu tasks also select their own cadence.
Neptune previously forced every native scene through a two-VBlank present
cap regardless of that request. A one-VBlank menu could therefore execute
at half its requested rate even when rendering was fast enough.

The pinned upstream main-menu Delete is an assertion-only stub. Read-only
Disc1 inspection confirms COMMON task definition20FA0C contains Init
06019C24, Draw06019D62 and Delete06019F18. Delete06019F18 reads byte1
and writes it back to0604BC98, the same native VBlank-count global referenced
by Map's standalone Delete. The missing restore is implemented through the
generated source adapter, leaving `extern/Azel` untouched.

## Change

- Capture the native requested cadence with the scene's presentation state.
  Publish a renderer-owned value through the existing frame-slot boundary.
- Native scene scanout pre-waits `N-1` VBlanks, accounting for display ticks
  already spent rendering. The existing NEXTFRAME/post-queue wait supplies
  the final tick. Request1 therefore does not insert the old extra tick;
  request2 retains the previous gameplay behavior.
- Existing native sources request1 or2. Unset/unsupported values fall back
  to2 rather than permitting a zero-tick spin or a huge display wait.
- Restore the saved main-menu byte on Delete, including forced Start exits.
  Native nested menus retain their own existing cadence choices/restores.
- The movie wrapper still requests2, and title's separate display/animation
  scheduler is unchanged. No Cinepak timing, texture filtering, radar,
  shader, task-count multiplier or synthetic catch-up update is introduced.
- `[NativeFrameCadence]` logs source request and selected interval only when
  the source changes, for a hardware timing check.

## Checks and hardware gate

`tests/menu_cadence_test.ps1` generates `mainMenuDebugTasks.cpp` through
the actual production CMake adapter, then executes its Delete body and
the actual renderer clock/staging functions with a mocked display clock.
Checks cover one-/two-tick presentation, menu entry/exit transitions,
16 one-tick menu frames, rendering that already consumed ticks, overruns,
counter wrap, invalid requests, the unchanged movie wrapper and saved-byte
restoration. The full generated menu source, renderer and scene bridge pass
Vita syntax checks. Publication/call-site checks are source guards, not GPU
or real scheduling evidence.

This removes an unconditional timing cap; it does **not** prove menus can
render in16.7ms. Heavy menus may still miss the requested cadence. No VPK
build, deployment, hardware timing or visual acceptance is claimed here.
Check opening/closing animations, cursor blink, save/load menus, nested
Dragon menus and return to ordinary flight. A fresh log should show request1
on entry and the saved request (normally2) after leaving. Report FPS/frame
timing separately from animation cadence; this is not a promised GPU speedup.

PowerShell build on the user's checkout:

```powershell
cd E:\dev\Lagi
git switch feature/menu-save-load-restoration
git pull --ff-only
cmake --build build-fullres
```

Map parent selection/resource/view hookup, Books and other unfinished menu
work remain separate; this timing fix does not complete the broader goal.

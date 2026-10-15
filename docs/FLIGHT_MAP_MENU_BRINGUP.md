# Flight Map submenu — Vita entry checkpoint

Branch: `feature/menu-save-load-restoration`

## Scope

The flight-mode Start → Display Map option now dispatches through Azel's
existing `createMapTask` slot to the Vita child task
(`src/integration/lagi_map_menu_vita.cpp`). The pinned upstream
constructor is an assertion-only stub; the CMake source adaptation marks
it weak without editing `extern/Azel`.

The child invokes the existing native COMMON-derived map ownership check
and Azel VDP2 map-menu backing setup. B exits the child; its Delete
restores the original status-menu VDP2 priorities and background setup.
The flight radar remains point-filtered and untouched.

## Current coverage

**Entry and exit only.** The screen shows native map-menu backing and
a temporary text status, not the field's actual map image. This is not a
completed map viewer. The available map helpers already recover native
field-map descriptors, source pattern names, exploration ownership,
pattern conversion, marker locations, map centering, and NBG2 window
register setup. Their task-level lifecycle is not yet connected, and
navigation is not yet enabled. Neptune now has the native-register-driven
NBG2 menu composition dependency (`MENU_NBG2_COMPOSITION.md`), but the
current entry child does not yet load/present the map planes to it.

The current implementation does not fabricate a map from the flight
radar or import the separate world-map overlay.

## Live context integration

The child now calls `lagiAzelResolveCurrentMapContext` at entry and owns its
result. This recovers the original current-field/dragon coordinates, native
multi-area zone transforms for fields0F/12, map slot, heading and SCB/PNB
descriptor. One `[FieldMapContext]` log records the resolved state. It does
not yet load/display the map or enable navigation. The existing placeholder
text remains temporary, not an accepted replacement for the native viewer.

Host tests exercise the production context service with mocked native
services across36 special-field routes plus normal/invalid state. Vita syntax
checks pass; neither full linking nor a hardware Map test has run here.
See `NATIVE_MAP_MENU_PORT.md` for source addresses and remaining dependencies.

## Hardware validation

The original field-view child and player-arrow command now have a compiled
task adapter (`NATIVE_MAP_VIEW_TASK.md`) with host lifecycle/packet checks.
The placeholder parent does not call it yet: parent selection, resource
lifetime/restore and the separate world-map route still require integration.

Build and test on a Vita after collecting the area's field-map item.
From flight mode, open Start, select Display Map, verify the map child
appears without a crash, and press B to return. Close Start and verify
flight controls and rendering resume. Also try entering before map
ownership and check the reported status.

The branch has not been built or hardware-validated as part of this
checkpoint. The source patch only establishes the native task boundary
for the remaining map viewer bring-up.

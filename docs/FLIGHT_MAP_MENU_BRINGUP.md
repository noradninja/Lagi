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
register setup. Their task-level lifecycle and Neptune's NBG2 map-plane
composition are not yet connected, and navigation is not yet enabled.

The current implementation does not fabricate a map from the flight
radar or import the separate world-map overlay.

## Hardware validation

Build and test on a Vita after collecting the area's field-map item.
From flight mode, open Start, select Display Map, verify the map child
appears without a crash, and press B to return. Close Start and verify
flight controls and rendering resume. Also try entering before map
ownership and check the reported status.

The branch has not been built or hardware-validated as part of this
checkpoint. The source patch only establishes the native task boundary
for the remaining map viewer bring-up.

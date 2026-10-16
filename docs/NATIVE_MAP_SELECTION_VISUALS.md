# Native Map selection visuals

The selection service now binds COMMON20E8A4 highlight addresses and
COMMON20E8B4 cursor configuration to the existing Azel cursor Update/Draw
callbacks and `switchVdp2MenuTileToHightlighted` routine. No replacement
cursor artwork, guessed blink animation, or custom palette conversion is used.

The adapter owns its decoded coordinate array for the task lifetime. A retained
shared state exposes selection, readiness and completion without requiring the
parent to dereference a deleted child. Selection outside 0..1 hides the cursor.
The native layer pointer is decoded into a checked layer index; highlight
addresses and their complete 3x3, 0x40-row-stride spans must lie in VDP2 VRAM.
Malformed configuration refuses initialization/highlight writes.

The world-choice cover service now owns a decoded configuration in a parent
task and creates Azel's actual `mainMenuTaskInitSub2` child through a checked
generated-source bridge. Native style0, slot1, artwork2118 and palette630 are
retained. No private work-area layout is mirrored in the adapter. The host
test extracts the bridge and native Init/Draw/factory from actual production
CMake output and checks owned coordinates, scrolling and rejected inputs.
The current Map parent still does not call this service; no visible change
or hardware lifetime/rendering result is claimed by this dependency alone.

## Integration boundary

This service is compiled but **not yet connected to the incomplete Map parent**.
It does not enable the Map selection page or world-map route. Native availability,
unavailable-slot covers, parent opening/closing lifecycle, field-view child hookup
and world-map confirmation/transition remain to be connected. Do not substitute
the field viewer for the world-map choice.

## Verification

`tests/map_selection_visual_test.ps1` extracts the production adapter and the
actual upstream native cursor callbacks and highlight routine. Host fixtures
check task-owned coordinates, layer scrolling, hidden/invalid selections, both
cursor sprites and native palette, blink callback boundaries, duplicate active
state refusal, cleanup, malformed configuration and palette-zero preservation.
The fixture uses the authored COMMON configuration values, not a captured device
frame. Mock tasks/VRAM/display do not validate Vita task scheduling or rendering.

The adapter also passed Vita C++20 syntax checking. No full VPK link, hardware
animation, visual acceptance or performance measurement is claimed.

PowerShell host check (from this checkout, with Visual Studio tools available):

```powershell
& 'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 -SkipAutomaticLocation | Out-Null
./tests/map_selection_visual_test.ps1
```

User build:

```powershell
cd E:\dev\Lagi
git switch feature/menu-save-load-restoration
git pull --ff-only
cmake --build build-fullres
```

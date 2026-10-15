# Native menu NBG2 composition

## Missing boundary

`setupVdp2ForMenu()` already configures NBG2 as 16x16, 4bpp, single-word
flip-capable pattern names, SCN12, CAOS6 and a window0/color-calculation
plane. Its authored map planes are A70800, B70000, C70000, D70800.
The native map-view service raises NBG2 priority to5 and animates CCRNB.
Neptune's menu compositor previously ignored NBG2 regardless of that state.

## Renderer service

The scene bridge captures raw NBG2 registers, window endpoints and signed
live scroll after staging the matching VRAM/CRAM/text snapshot. Producer
and renderer each own a value copy. Every new text snapshot clears pending
NBG2 first; only the native status-layout capture attaches the matching state.
Movie overlay publication clears the published menu plane. Save layout
explicitly excludes this menu-only decoder.

`MenuNbg2State` retains native register values. The portable helpers decode
all four map-plane addresses, signed scroll/wrap, inclusive window0 bounds,
layer priorities and RGB555 color calculation. The pixel consumer reuses
Neptune's existing native 16x16 character/flip and palette decoder. SCN and
CAOS come from the snapshot, not a resource filename or menu ID. Format
guards reject other character/name/plane formats and unsupported window1,
sprite-window, opaque-zero or alternate calculation modes.

For ratio mode, the original five-bit register selects top:second weights
`(31-ratio):(ratio+1)`, denominator32. This is performed in RGB555 before
expanding to the existing ABGR output, not an approximate 0..255 alpha.
Source: [Sega VDP2 User's Manual, color calculation](https://www.infochunk.com/saturn/segahtml_en/hard/vdp2/hon/p12_11.htm).
NBG0/1/2 priority and transparent pixels determine the top/second source;
NBG3 text remains in its existing later pass. This supports the native menu
configuration, not arbitrary VDP2 special/extended color calculation.

NBG2 joins the existing source-raster menu background resolve. This adds no
texture, allocation, shader or draw call. There is extra CPU decode/blend
work only when a supported NBG2 plane has nonzero priority. Ordinary menus
with priority0 retain the existing path. Frame timing has not been measured.
Current dither sampling and radar point-filter exception are unchanged.

## Checks and unfinished integration

`tests/menu_nbg2_test.ps1` extracts and executes the production NBG2 pixel
consumer, RGB555 expansion and existing character decoder. It checks four
planes, signed scroll wrap,1024 pixel/flip cases, transparent dots, both
window0 areas and inclusive boundaries,32768 RGB555 blend combinations,
3584 layer-priority/presence cases and unsupported-state rejection. Transport,
movie reset and save exclusion have source guards, not runtime/GPU proof.
Vita syntax checks cover the renderer and scene bridge.

This is a display service dependency, not a completed Map page. The active
Map child still needs its real native parent/view lifecycle, resource loading
and restoration, pan/selection/fade input and player/orientation marker.
No VPK build, hardware rendering, timing or menu acceptance is claimed.

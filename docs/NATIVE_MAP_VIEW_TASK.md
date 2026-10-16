# Native field-map view child

## Original source and implemented task

`lagi_map_view_vita.cpp` ports the original MAIN child constructor06017670,
Draw060174C8, Delete0601763C and player-marker Draw06016FF8. It is a native
task adapter and VDP1/VDP2 service consumer; no radar mesh, replacement map
or approximate page is generated. The task is compiled into the Vita target.

`lagiAzelCreateFieldMapView` requires a parent and explicitly ready source:
native context, both authored PNB halves
and a retained completion object. It copies context/patterns at Init. Thus
the original source buffers cannot become dangling when a loader releases
its temporary memory. The parent must retain resource/graphics ownership
until its view child has been deleted. A shared status object reports setup
and completed cleanup without requiring parent dereference of a freed task.

The task implements all original phases0..7:

- Clamp initial signed map coordinates/32 to signed descriptor bounds
  `[byte8+2,byteA-3]` and `[byte9+2,byteB-3]`.
- Convert the two PNB halves with the native ownership/exploration services,
  establish NBG2 scroll/window/priorities and retry pending setup as native.
- Reveal over16 Draws, preserving CCRNB upper bits. The final reveal tick
  makes the player marker visible and falls through to active input/draw.
- B has priority over movement. Horizontal/vertical pan uses the same
  original clamped-index branch ordering, held/repeat input and boundary
  sound latches as the shared native Books index service. These are bound
  to `graphicEngineStatus.m40AC.m6/m7` rather than independent task latches.
- Pan steps1..7 each draw decorations at `(delta*32*step)/8`; after step7,
  the same Draw snaps to the final origin and returns to active phase2.
  The player subtask-equivalent is emitted after that snap, as in native
  parent/child order. This is the original eight-step sequence, not an
  eight-Draw wait followed by an extra snap.
- B initiates the native16-Draw close, hides the marker on close setup,
  reaches ratio31, then finishes. Delete restores native menu priorities
  before reporting completed cleanup to the parent.

Boundary arrows read the original four ten-byte COMMON records at20E8BC,
including their CMDCTRL texture-flip bits. Object markers use the existing
native flag/position/clipping service. The player arrow is the original
textured distorted-sprite command1002, PMOD80, palette700, SRCA23D0,
SIZE0220. Its four corners use native sine/cosine at negated heading,
including the original forward displacement and asymmetric rear edge.
Commands enter the normal VDP1 packet stream; Neptune's existing UI command
capture services them. No custom draw pass or shader is added.

## Validation and remaining parent integration

`tests/map_view_task_test.ps1` compiles the actual task/factory/command emitter
and actual shared index routine with mocked native task allocation, buffers,
VDP2 services and cardinal sine/cosine. It exercises setup retry, copied source
lifetime,16 reveal ticks, packet fields/corners and texture flips, cardinal
heading, horizontal/diagonal pan, boundary latch, cancel precedence,16 close
ticks, cleanup completion and malformed/unready source rejection. It verifies
the last pan's object-marker position versus the subsequent final arrow snap.
This is production control-flow/packet evidence, not native trigonometry
differential testing, real task scheduling or rendered-image evidence.

Vita syntax checks pass. No full link, VPK, deployment, hardware image or
performance measurement is claimed.

**The existing top-level Map entry remains a temporary placeholder.** It
does not yet call this factory. It must still be replaced by the original
Map parent's selection/mode/cursor/scroll lifecycle, with an owned resource
handoff and both native field-map
and world-map routes. Connecting this child through an automatic field-only
shortcut would not complete that original parent workflow. The map viewer
and the broader in-game menu goal therefore remain incomplete.

The newer parent resource owner now loads the descriptor's SCB/PNB and
restores overwritten spans on failure/exit (`NATIVE_MAP_RESOURCE_OWNERSHIP.md`).
It does not yet construct this view child or replace the placeholder parent.

The view now binds to validated live native field/slot/descriptor state rather
than requiring a fabricated subfield count (`NATIVE_MAP_CONTEXT_BINDING.md`).
Arbitrary indexed service callers retain their original count checks.
# Parent completion observation (2026-10-16)

MAIN060178B6..060178C4 reads the child task header's finished bit, not
completion of its Delete callback. The portable view status now becomes
finished at the same point as `markFinished`, including invalid-init paths.
Priority restoration and task cleanup remain in Delete; `finished` does not
mean those side effects have already occurred. Delete still publishes a
finished status for external teardown. This prevents a future parent hookup
from adding a frame solely because it waits for deletion instead of the native
task flag. Production-body host tests verify close completion is visible
before priority restoration and invalid context/bounds finish immediately.
Vita syntax passes. The full parent hookup and on-device Map validation
remain unfinished; this does not alter the current menu performance-test path.


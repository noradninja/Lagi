# Unresolved garbled load transition

User reports successful loading but a garbled screen where a fade is expected.
Recording 2026-10-11 16-40-13.mp4 was visually inspected via a one-second
contact sheet. After save-slot text disappears, a narrow multicolored tile
strip near the bottom and a small central fragment persist over black for
several seconds, then gameplay appears. This is now visually verified, but
the precise native state at those frames is not yet matched to a log.

Source evidence: loadDrawSub1 restores the save, then increments m4_status to 2.
DrawLoad state 2 clears menu/text and scrolls for ten updates; state 3 commands
both native fade channels to black (0 to 0, one step); state 4 finishes. The
renderer excludes nativeMenuFrame from its final scene color-offset pass.
Neither fact alone proves the cause of the garbled tile data. Do not mask it
with a substitute fade or change the native transition order without evidence.

Renderer audit: drawAzelMenuVdp2Gpu decodes NBG0/NBG1 backing and NBG3 glyphs
directly to ABGR without applying native per-layer CLOFEN/CLOFSL offsets.
The final scene offset excludes nativeMenuFrame, so native menu layers do not
have the same fade presentation as ordinary scene composition. This is a
confirmed missing rendering behavior, not yet proof that it alone causes the
observed strip (map lifetime/scroll still need checking). Repair should apply
each layer's native selected channel while preserving transparency and layer
priority, not place an unconditional black overlay over the entire screen.

New radar-crash log uploaded at 20:42 also includes the load trace. State 2
and 3 retain A/B=(0,0,0); state 4 reaches A/B=(-128,-128,-128), counter=0,
stopped=0. CLOFEN=0000 and CLOFSL=0000 throughout states 0..4. Therefore a
strict enable-gated per-layer offset implementation alone will NOT restore
this transition. Map registers remain BGON=000F CHCTLA=1111 CHCTLB=0001
MPOFN=0311 MPABN0=0040 MPABN1=0141 MPABN3=0303 at every state change.
Native setup/CLOFEN reconstruction and scene handoff need checking before
selecting a correction; do not treat the earlier missing-offset hypothesis
as a complete diagnosis. The current test build remains 8b9cf45 unchanged.

Separate original-code audit of clearVdp2TextAreaSub1 (06025EB8) proves three
reconstruction errors: ownership lookup must be x*64+y (06025EDA..06025EEC),
both x and y must normalize by the glyph's low-two-bit cell encoding
(06025F00..06025F0E), and ownership columns are 64 u16 cells apart, not 128
(06025F10..06025F52). Generated VDP2 source now repairs those operations and
clears all four ownership cells before the existing four row-major VRAM writes.
This also corrects the VRAM origin when clearing from a right/bottom glyph cell.
No fade request/order was changed. glyph_clear_test.ps1 passes patched-source
Vita syntax and 15,876 origin/index model cases. Those models do not execute
the native function; visual cleanup and the corrupted strip remain unverified.
This repair is not part of the user's 8b9cf45 hardware test.
Additional host check executed the extracted patched production function with
4096-cell ownership/VRAM stubs: all 15,876 origin/subcell combinations cleared
exactly the four expected cells and released one glyph reference; empty/equal
branches also passed. This is actual function execution on a host, not Vita
execution and not proof of the full transition's visual behavior.

Generated-source logging emits at most 24 state/operation changes per launch:
`[LoadTransition]` reports selected device/slot, restored version, CLOFEN/CLOFSL,
both color offsets and primary fade countdown/stopped state.
`[LoadTransitionMap]` reports layer enable, character-format and map-address
registers at the same changes. No save writes, timing/state mutations, or
per-frame logging are added.

Next hardware check: load a valid save, retain the matching lagi.log, and capture
the transition. Compare the first garbled frame with state 2/3 and the published
map/fade configuration. Patched native code passes Vita syntax checking only;
the transition is not declared fixed.

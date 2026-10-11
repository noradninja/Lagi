# Unresolved garbled load transition

User reports successful loading but a garbled screen where a fade is expected.
This has not been reproduced or visually verified in this workspace.

Source evidence: loadDrawSub1 restores the save, then increments m4_status to 2.
DrawLoad state 2 clears menu/text and scrolls for ten updates; state 3 commands
both native fade channels to black (0 to 0, one step); state 4 finishes. The
renderer excludes nativeMenuFrame from its final scene color-offset pass.
Neither fact alone proves the cause of the garbled tile data. Do not mask it
with a substitute fade or change the native transition order without evidence.

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

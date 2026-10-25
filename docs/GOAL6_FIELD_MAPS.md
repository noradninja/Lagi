# Goal 6: field-map lifecycle

Status: active; no hardware acceptance or runtime correction claimed yet.

Required acceptance: first and second map pickups, visible menu entry and
selection, native blue visited-area markings, normal cancellation, and the
same ownership/exploration after saving and reloading. Preserve accepted
rendering and performance. Do not modify extern/Azel.

## Current source audit

`lagi_map_menu_vita.cpp` falls back to a placeholder when parent availability
cannot be resolved or its enabled world destination has no backend. The
placeholder displays a selection message but never creates a field viewer.
This is a candidate for the invisible menu, not a confirmed reproduction.

`lagi_map_parent_source_vita.cpp` explicitly rejects nonzero authored field
routes while native low-RAM gate 0024B89C is unresolved. The module-manager
generator still leaves entry 5 (CHANGE.PRG) null; entry 6 (WORLD.PRG boundary)
does not substitute for it. Enabling the world slot without its native owner
and actual backend would not be a correct field-map fix.

Exploration reads use COMMON20A388[field], subfield record, signed rectangle,
and the signed word at record+8; native MAIN0601249C confirms the offset.
They query the existing getBit566 state, not a separate renderer cache.
The shared producer of explored cells still needs tracing. A3 exit bits alone
are not evidence that all map cells are marked.

## Newly established native gate producer

Read-only Disc 1 MAIN inspection identifies **06012434** as the setter:

- 06012434 loads the literal at 06012530: 0024B89C.
- 06012436 returns with delay-slot MOV.B R4,@R3 at 06012438.
- MAIN06011BB0 loads setter pointer 06012434 from literal 06011C30.
- 06011BB2 calls it with delay-slot R4=1 at 06011BB4.
- This occurs in initialization beginning 06011B90, immediately after the
  new subfield pointer is stored at field+8 and subfield+370 is cleared.

An aligned MAIN scan found the setter pointer at 06011C30 and no direct BSR
to the setter. This is not a whole-disc caller proof. The source counterpart
and other overlay/script writes must be established before binding the live
flag; it must not be replaced with map ownership or a guessed story bit.

Evidence: original AZEL_1.bin MAIN LBA58, 253952 bytes, base06006000;
`disc_sh2_inspect` ranges 06011B90..06011C40 and 06012434..06012554.

## Outstanding

Trace shared exploration updates, map pickup state and native gate lifetime;
reproduce the second-map path with fresh diagnostics; implement narrow
source-faithful corrections and tests; build Vita; obtain pickup/menu/save
and reload validation on hardware. Host tests alone cannot close this goal.

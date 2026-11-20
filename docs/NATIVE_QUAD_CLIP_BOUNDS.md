# Native visible rejection and guard-band quad clipping

Follow-up audit while restoring Tutorial2's animated line effect. No upstream
edits. This corrects the shared recovered world-ray/world-polygon service,
not the previously accepted shaded battle-ray path.

Original MAIN0602D946..0602D992 and A7 polygon060699FC..06069A4E pass two
boundaries to each screen-plane callback: R5 receives the visible bound from
clip offsets C/E/8/A, R6 the wider guard bound from4/6/0/2. The initial16-case
mask uses R5. Wholly outside/unsupported mask cases reject at that stage.
For accepted mask actions, screen edge helpers0602C9F0/C9FE/CA44/CA52 test
against R6; a classified outside vertex already inside the guard band is left
unchanged. Only guard-band crossings use the signed8-step intersection.

The first recovered service used one guard boundary for both decisions.
That retained wholly offscreen primitives and omitted native visible-mask
actions. Clamping all intersections to the visible rectangle would also be
incorrect: accepted shapes intentionally keep vertices between the visible
and guard boundaries. A new shared two-boundary entry restores the actual
contract; the equal-bound API is retained for existing equal-bound consumers.
Near-depth clipping still uses its one near boundary and unsigned division.

Both recovered world wrappers now pass the live native bounds in the original
right/left/top/bottom sequence. Native XY/material/sort keys and renderer-only
depth ownership are unchanged. Skipped guard intersections also leave depth
metadata untouched. No geometry approximation, new allocations or GXM passes.

The production helper fixture covers all16 masks on both screen axes/sides,
six positions between/beyond/on the guard boundary, signed-fraction rounding,
unchanged/rejected ownership and metadata. Wrapper tests include wholly
offscreen-but-inside-guard rejection and partially visible shapes whose guard
vertices must remain unchanged. Existing near/far, projection, packet and
capacity checks remain regression gates. Hardware visual/timing acceptance
remains pending; Tutorial2 is still not enabled.

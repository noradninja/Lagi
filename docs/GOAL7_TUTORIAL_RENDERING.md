# Goal 7 tutorial rendering checkpoint

Hardware report: first Tutorial 1 section runs, but weak markers overlay the
dialogue panel, backgrounds are ordered incorrectly, and frame rate is 20 FPS.
The fresh log enters BTL_T0.PRG, overlay 15/sub 0. Steady samples have about
33-35 ms render time, 22-25 ms GXM finish, cache-hit sprite textures and no
static rebuilds. These are observations, not proof of a single GPU cause.

## Native packet order

PDS.cpp flushVdp1Sub2/4/5 and flushVdp1Sub3 perform stable descending radix
sorting of the low 15 packet-key bits. Lagi captured those keys but published
commands in emission order. The shared publication boundary now performs the
same stable ordering; equal keys preserve emission order and commands without
packets remain zero-key overlays. No texture IDs or tutorial-specific sorting
rules are used. The host fixture covers all 65536 keys, stable ties and an
emission-order panel/marker/background example. Vita package build passes.

This fixes the missing VDP1 ordering contract, not every VDP2 composition
priority band. Hardware acceptance of the panel/markers remains outstanding.

## Remaining investigation

The T0 rolled/tiled background emitted a native farClip-1 bucket but omitted
the normalized extended depth used by the existing A3 background helper.
It now attaches the same renderer-only metadata, leaving native geometry,
material and packet keys untouched. This routes the sprite through the
existing Background pass rather than foreground. The production-include
fixture checks the far-depth metadata as well as all previous coordinates,
materials, buckets and guards. Host tests and Vita package build pass;
device ordering and performance are not yet accepted.

The tutorial RBG cache reports reason 5: OVPNR screen-over pattern mode is not
yet supported by decoded pages, so it takes the complete raw decoder. Add
general resource support without dropping screen-over semantics. The current
log does not isolate that shader's exact GPU cost. Keep VDP2 priorities,
normal-combat acceptance and quadrant visibility as separate checks.

## Screen-over acceleration implemented, device acceptance pending

The decoded-page route now accepts OVPNR mode 1. Ordinary map pixels use
the existing indexed page atlas. Outside samples use the authored OVPNR
register directly with native 16x16 character addressing and the live raw
character/CRAM texture. Pattern-name RAM permissions do not apply to this
register-selected name; character-bank permissions, transparent zero, CAOS,
palette wrapping and flips still apply. No additional textures, cache slots,
GXM scenes or approximate replacement effects are introduced. Unsupported
formats, page capacity and allocation failures retain the complete fallback.

The production scalar shader address helper runs against the original Azel
tile decoder for all 65536 pattern words, eight supplement settings, every
16x16 dot and signed-repeat coordinates. Production cache-service and shader
switch fixtures pass. Actual compiler reflection passes for generic, byte,
page and row variants (including live OVPNR uniforms in row mode). Vita
package compilation passes. These checks are not hardware frame-time proof.

The next device log should show the tutorial using RBG0PageCache active=1
instead of reason=5, and RBG0TileFormat decodedPages=1 genericFallback=0.
Compare output and frame times in tutorial and normal field/combat/name entry.

The user confirmed background ordering corrected on e2bfcb7, but weak-point
icons/text still overlap. Do not mark that fixed or conflate it with quadrant
visibility: the two remaining contracts need separate native-state tracing.

## Native indicator packet keys recovered

Original US Disc 1 BTL_A3.PRG (LBA 258144, 385044 bytes, base 06054000)
reticle draw 06069094 and weak-marker draw 0606970C assign their sprite
packet keys by loading graphicEngineStatus+4094 (far reciprocal), SHLL8,
SHLL2, SHLL2, then logical SHLR16. Examples: 060691A6..B2,
06069886..92 and 06069A56..62. This is unsigned 32-bit wrapped arithmetic,
not GXM geometry depth. The upstream targetable source instead assigns
zero at all nine sprite packet sites. A guarded generated-source adapter
restores the calculation at exactly those nine boundaries, retaining the
retirement guards, native visibility/state gates, materials and coordinates.
Combined with stable packet ordering, indicators now precede zero-key panel
sprites even when their task emits later. Native VDP2 layer priorities remain
a separate compositor requirement; do not call the whole overlap fixed yet.

The production helper is tested over the full low 20-bit domain and 100000
32-bit overflow cases against the SH2 instruction sequence, plus shared
packet-sort integration. Generated output must contain nine replacements
and no remaining zero-key sites. Hardware confirmation remains required.

Native weak-marker update 060695F0..0606970A agrees with the upstream gate:
retired bit40000 finishes the task, disabled bit100000 clears bit200000,
and absent bit200000 suppresses drawing. It does not directly test a quadrant.
Trace the producer of bit200000 and the selectable-target/quadrant contract
rather than inventing an extra marker-only filter. Normal target cursors use
the selectable index m5A; weak-marker visibility is a distinct path.

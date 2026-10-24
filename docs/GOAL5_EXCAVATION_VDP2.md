# Goal 5: Excavation ground corruption

Active 2026-10-24. Goal4 is accepted. Both E006 and Excavation use
startExcaBackgroundTask; original Saturn reference has a black ground region.
Do not force mapA to60800 or mask an Excavation scene in the renderer.

## Current evidence

Fresh user log (Goal4 acceptance run) contains both mode1 and mode2:
BGON001A, RPMD2, PLSZ0400, OVPN0000/0000, mapA00000, mapB60000.
The generic shader currently reads any raw VRAM address as pattern data,
without RAMCTL rotation-bank access state. FrontendRbg0State does not carry
RAMCTL. The renderer cannot currently distinguish rotation-bank data from
unrelated UI/normal-scroll data at the same address.

Original TWN_EXCA.PRG, Disc1 LBA235981, size75640, base06054000:

- 06055DD0 literal references the sky table06064B94 (all25E60000).
- Adjacent table06064BD4 (all25E60800) has no absolute pointer reference in
  the program. Its mere presence is not evidence to activate it.
- 06055DCA..06055DCE literals support the initializer's RAMCTL high-byte
  preservation and low byte B4; the normal-scroll cycle pattern is13FF57FF.
- 06055E1C..06055E3A fills25E60800 using50005000, 0x200 iterations of16bytes.
- 06055E3C..06055E42 sets OVPNRA to0, matching the reconstructed initializer.

The existing one-word/16x16/256-color setup and prior big-endian pattern-fill
correction remain intact. Both raw and decoded-page GPU routes must retain
their accepted sampling/performance behavior.

## Shared bank-access lead, not yet a verified fix

Sega's original VDP2 manual, section6.2, defines RAMCTL bits7:0 as four
two-bit rotation-bank roles:0 unused,1 coefficient,2 pattern names,3 character
patterns/bitmap. Unsplit banks use the A0/B0 role. It states that image data
outside the assigned bank is not read. Reference mirror:
https://www.infochunk.com/saturn/segahtml_en/hard/vdp2/hon/p06_20.htm
Bank splitting is controlled by bits8/9 (section3.2):
https://www.infochunk.com/saturn/segahtml_en/hard/vdp2/hon/p03_20.htm

For split banks and low byteB4, A0 is not rotation RAM, A1 is coefficient,
B0 is character data, B1 is pattern names. MapA00000 points into A0 while
mapB60000 points into B1. Sampling A0 unconditionally can expose unrelated
VRAM as the observed ground corruption. This is stronger evidence than the
previous unused-map-table hypothesis, but the manual's no-read restriction
alone does not specify a universal pixel value for every illegal fetch.

Next: verify published RAMCTL against the native setup, define general
no-fetch handling with the black reference as the acceptance case, and cover
both raw sampling and decoded-page cache paths. Prefer per-frame/per-page bank
validation over extra per-pixel tests or another GXM pass. Retain screen-over
pattern semantics: an out-of-map OVPNR fetch does not read pattern-name VRAM.
Hardware acceptance requires E006 plus town floor, and unchanged name-entry,
field water/sky and combat performance. No rendering change in this audit.

## Native reset prerequisite verified

MAIN06032292 calls shared reset060322DE, which calls06032320 for both native
register copies. At06032348 opcode9049 loads word1300 from060323DE. OpcodeC203
at0603234A stores the resulting long00001300 atGBR+0C: VRSIZE0, RAMCTL1300.
The reconstructed resetVdp2RegsCopy instead memset-clears RAMCTL and never
restores it. Excavation therefore reconstructs00B4 instead of native13B4.
Enforcing bank roles before correcting this would misclassify valid B1 data.

The generated shared reset now restores RAMCTL1300 after clearing registers.
No overlay-specific reset value or extern/Azel edit. The disc-backed reset
test checks the actual original load/store/literal and generated default for
both register banks. This is a prerequisite, not the floor fix: Neptune still
needs native bank state and no-fetch handling in raw and cached sampling.

## Shared rotation image-bank access implementation

RAMCTL now travels in the existing native RBG frame snapshot for both scene
and frontend producers. No overlay/status checks or forced map addresses.
One shared helper resolves the four physical bank roles including unsplit
A0/B0 aliases. Raw generic/specialized shaders receive two small per-frame
permission vectors and avoid inaccessible pattern-name and character fetches.
OVPNR is a register, so its out-of-map name bypasses the name-bank check;
the resulting character fetch still requires a character bank.

The service represents an unavailable image fetch as no RBG coverage, exposing
the authored back screen (black here), rather than sampling unrelated memory.
This is the testable no-fetch interpretation for the provided hardware case,
not a claim that Sega defines every invalid bus-read result as transparent.
The final hardware comparison must confirm it. Coefficient access remains
the established coefficient path; this change handles rotation image data.

Decoded-page keys include RAMCTL and enforce image-bank access on rebuild.
Unavailable name pages cache zero coverage and do not depend on unrelated
name/character bytes. Unavailable character tiles likewise avoid data reads;
cross-bank character tiles check each addressed dot. Register-only role
changes rebuild the page even when VRAM generations are unchanged. Indexed
palettes remain live on GPU. No new draw pass, readback, or texture allocation;
cached/row shaders compile out the bank uniforms and perform no additional
per-pixel bank checks. Raw paths do have new access tests; measure hardware
timings instead of claiming zero cost from build success.

The production program-switch test now covers both bank uniform bindings,
their removal in cached programs, all generic/specialized/cached/row switches,
missing uniforms, creation failure and cleanup. The bank test exhausts32768
role/address-edge cases, verifies native13B4 permits names60000/characters40000
but not names00000, and tests inaccessible pages and register-only invalidation.
The original native decoded-page pixel/flip/palette/invalidation oracle passes.

Expected fresh diagnostic: RBG0BankAccess RAMCTL=13B4, mapA00000/nameRoleA0,
mapB60000/nameRoleB2. At most16 register-transition reports per launch. This
confirms service inputs, not visual correctness. Request a fresh E006/Excavation
log/video plus a brief name-entry/field/combat check before accepting Goal5.

Final implementation checks pass: original-disc reset test,32768 bank cases,
native decoded-page pixel oracle, production shader switching/failure/cleanup,
generated Excavation big-endian fill, git diff whitespace check and full Vita
VPK build including generic, tile16, decoded-page and coefficient-row fragment
programs. No extern/Azel changes. The GCC7.1 parameter-passing notes are compiler
notes, not failed compilation. No hardware claim until the next user run.

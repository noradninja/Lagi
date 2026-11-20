# Shared native model color animation

## Hardware acceptance

2026-10-24: user confirms the effect after testing 3010904 and supplies a fresh
lagi.log. NativeColorAnimation reports eight valid submissions, 11 polygons,
1-3 sparse updates each. The eight following tunnel windows cover 960
presentation intervals at 29.970 FPS, zero over two VBlanks; renderer-window
means 10.602-12.220 ms, maximum 23.239 ms. Combined with the user's visual
confirmation, Goal 4 is accepted. This does not prove a matched global
performance improvement or eliminate the separately recorded outdoor misses.
Pending hardware language below describes the original implementation handoff
and is superseded by this acceptance.

Goal 4, 2026-10-24. Tunnel audio is accepted by the user; the remaining switch
color/gloss effect must use the general native pipeline, not a visual proxy.

## Root and original executable audit

The A3 generator initializes hierarchy table offset40 from FLD_A3_0.MCB,
selects color streams at table slots300/304/308, and advances model m14 through
func3dModelSub0. Running/shutdown/startup frame selection is already authored.
initModelDrawFunction selects variant9 for its non-scaled/no-hotpoint pose.
That reconstructed variant explicitly says its colored draw is TODO, then
calls the ordinary uncolored traversal. Other colored variants0/2/5/7/11
are unimplemented. Thus the data is lost before reaching Neptune.

Original Disc1 MAIN mode1/2352 extent LBA58, size253952, base06006000:

- 06020084..0602011A selects matrix/pose, scaled, hotpoint and colored variants.
- 060201EC selects 060206DE for the colored pose/no-hotpoint branch.
- 060206DE..060207AC indexes color stream+4's frame table using model m14,
  then traverses one color-record pointer per bone in preorder alongside poses.
- 0602090E..060209F0 is the equivalent scaled pose path; matrix/hotpoint paths
  use the same frame/bone selection. Colored variants retain object-only
  submission; no invented billboard branch.
- 0601E3C6 supplies the selected bone's animation to the normal object draw.
- 0601E83C..0601E8CE matches sorted polygon indices against 16-byte records:
  index32 followed by four signed BGR byte triples; FFFFFFFF terminates.
- 0601EA3C..0601EAD6 adds sign-extended bytes shifted8 to distance falloff and
  positive normal/light diffuse, clamps to0..1F00 and packs B-high/R-low RGB555.
  This replaces material color offsets on listed polygons, not their normals
  or directional lighting. Unlisted polygons retain their ordinary shading.

MCB offsets remain stream-relative in host memory (not relocated Saturn
pointers). nativeColorBone resolves them explicitly without mutating MCBs.
Like the native model loader this trusts loaded asset extent ownership;
negative/out-of-range frames and invalid/non-increasing polygon indices are
rejected. Disc tests separately verify every used record stays inside the file.

## Shared implementation and GXM cost

Guarded generated mainMenuDebugTasks adapters implement all six colored draw
variants using one hierarchy helper: matrix/pose/scaled transforms, preorder
bone indexing, hotpoint outputs and normal stack lifetime remain native.
No extern/Azel modifications or generator-specific rendering condition.

The bridge decodes only sparse records into retained-capacity producer storage.
Publication copies that small payload alongside its matching submissions;
retainPreviousScene retains both together. No live asset pointer crosses the
renderer boundary. The immutable adapted-model cache remains unchanged.

Neptune applies the sparse offsets directly to its final per-instance polygon
descriptors during the existing dynamic append. Single-normal polygons expand
to four equal normals with independent color corners. Existing lit Gouraud
preparation/payload and GXM draw consume the result. No extra shader variant,
pass, texture update, cache invalidation, synchronization, or mesh re-adaptation.
This is a source-level cost argument, not measured hardware performance proof.
The bounded NativeColorAnimation trace reports at most eight nonempty/invalid
submissions per launch; it does not log every frame.

## Verification and remaining acceptance

tests/native_color_animation_test.ps1 reads the original disc, derives actual
hierarchy/polygon counts and validates all three authored generator streams:
84 frame/bone samples, 682 sparse records, 3784 nonzero channels. It tests
signed/asymmetric BGR->RGB conversion, single-normal expansion, unlit exclusion,
duplicate/out-of-range polygon rejection, invalid frames and generated dispatch.
The production hierarchy helper fixture covers matrix/pose, scale/no-scale,
hotpoints/no-hotpoints, child/sibling preorder, balanced stack and billboard
exclusion. Vita VPK compilation is an additional gate, not visual acceptance.

Hardware acceptance remains open: compare running switch sheen/color motion
with Saturn; turn it off and back on to check the authored shutdown/startup
sequences and settled state. Capture fresh lagi.log plus video, including nearby
ordinary tunnel gameplay and the switch. Compare frame-time windows, GPU wait,
append/build work and texture/cache activity; retain the accepted combat
baseline and do not call the absence of new passes proof of zero regression.

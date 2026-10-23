# Approved disc-selector design (960 x 544)

This implementation follows the user-approved 960 x 544 review composite from
23 October 2026. The upper-right frame has the continuous rounded contour;
the status rows remain open on their left and clear of the straight right rail.
The information plaque joins the lower frame and contains all of its text and
control icons inside the dark inset.

## Runtime assets

`assets/frontend/png/` contains 19 independently usable transparent PNGs.
The matching `.rgba` files in `assets/frontend/` have an 8-byte little-endian
width/height header followed by tightly packed RGBA8888 pixels. Each PNG and
raw payload is pixel-identical. The original, unscaled disc medallions are
preserved in `assets/frontend/source_portraits/{png,rgba}/`; the four 71 x 71
runtime portraits are derived from those same images for a 1:1 blit.

The screen uses the existing `ASCII.CGZ` decode and PDS palette at runtime;
no font or game BIN/CUE is bundled. The row and heading glyphs use 10 x 22
cells, while the control labels use the native 8 x 16 cells. O/X and the Cross,
D-pad, and left-stick prompts are separate transparent sprites.

## Layout contract

| Element | Destination |
| --- | --- |
| Background, frame, radial | (0, 0), 960 x 544 |
| Radial portrait centers, discs 1-4 | (254,108), (404,270), (254,402), (99,270) |
| Status plaques | x=515; y=75,150,225,300; 380 x 68 |
| O/X markers | plaque offset (+25,+10); 48 x 48 |
| Information plaque | (400,370); 560 x 155 |
| Cross, D-pad, left-stick prompts | (545,453), (545,472), (583,472); 20 x 20 |

The available/absent states, selected portrait tint, and continuation disc
number remain dynamic. The existing launcher frame cache, GXM presentation,
navigation, fade, BGM, external CUE/BIN probing, `app0:frontend/` loading,
and Azel/Neptune responsibilities are unchanged.

## Verification and Vita handoff

- Host frontend font/selector test passes, including native decoder
  differential, rendering cache, selection states, clipping, and fallback.
- Font loading test passes for loose, disc, missing, and invalid sources.
- The compiled host selector frame differs from the approved composite by no
  more than 2 levels in any color channel; this is alpha-blend rounding.
- All 19 PNG/raw pairs match pixel-for-pixel, all transparent pixels have zero
  RGB, and the four original medallions match the prior branch assets.
- Vita build and on-device visual/input/audio checks are **not** claimed here.

Build with `LAGI_FULLRES=ON`, install the VPK, and inspect the selector at
960 x 544 on Vita/Vita TV. Verify available and absent discs, the centered
O/X markers, the rounded upper-right corner, both lines of the information
plaque, navigation/confirm, BGM, fade, and launching an external CUE/BIN.

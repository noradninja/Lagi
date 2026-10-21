# Cinematic rendering acceptance

## Current evidence

The user has accepted shared sprite/menu fades, post-death weak-point removal,
and Edge's post-boss subtitle. Keep these as regression checks; do not replace
them with title cards, local boss filters, or layer-specific fade guesses.

Sand occlusion (`4f08b2a`) enables native geometry depth for town/cinematic
modes as well as field/battle. Projected particles retain native integer-pixel
X/Y and carry actual view Z, not sorting bias, to the renderer. Host tests
cover the production particle adapter and scene-mode capability, not GPU
occlusion. The corrected projection guard (`d848b0c`) permits float view-Z
metadata without permitting fixed-point X/Y unit conversion regressions.

Cloud transport (`80af1fa`) preserves all 224 native line rows in Saturn byte
order. Accelerated NBG0 (`e95ca80`) decodes native tile/line/window state on the
GPU at 352x224, then uses ordinary filtered composition. It does not use
stochastic sampling. Host scalar shader tests cover addressing and windows;
they do not prove Vita shader output or timing.

The available Downloads/lagi.log dated 2026-10-21 13:53 predates these cloud
changes and contains no SceneNBG0 markers. It cannot validate the new route.

## Next device test

Use the latest feature/battle-mode-bringup build. Play the entire pre-boss
cinematic without skipping, then defeat the boss:

1. Confirm sand behind mountain geometry is occluded while exposed sand stays
   visible. Capture both normal and wire views if the mismatch remains.
2. Compare the transparent moving cloud layer with the Saturn reference.
   Confirm the background remains visible and there is no sprite-style
   stochastic noise on NBG0.
3. Preserve a fresh log with SceneNBG0State and SceneNBG0 records. The latter
   should report nativeGpu=1, ok=1 and stochastic=0 while clouds are enabled.
   A successful record proves submission, not correct pixels.
4. Confirm weak-point markers disappear on death and Edge's line remains a
   subtitle. Check fades still include sprites.
5. Capture frame timing through the cloud sequence. Target 25 ms, deadline
   33 ms; CPU submission timing alone is not isolated GPU cost.

Cloud and sand acceptance remain open until this device evidence exists.

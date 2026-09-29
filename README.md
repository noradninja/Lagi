# Lagi

**Lagi** is a native PlayStation Vita reimplementation of *Panzer Dragoon Saga*, adapting reconstructed game logic and Saturn systems to VitaSDK and native SceGxm.

The goal is not to emulate a Sega Saturn. Lagi aims to execute reconstructed PDS game logic natively on the Vita's ARM CPU while translating the game's platform, rendering, input, audio, and storage requirements to Vita hardware.

> **Status:** active native Vita bring-up. The Azel task runtime, real Disc 1 BIN/CUE access, COMMON.DAT parsing, dragon metadata, DRAGON0.MCB hierarchy/hotpoints, and Basic Wing geometry decoding are working on real hardware. A native SceGxm Basic Wing debug viewer is now being brought up.

## Project lineage

Lagi builds on several community reverse-engineering efforts:

- **Azel** — primary reconstructed/reimplemented PDS runtime and initial source-level foundation.
- **ATOLM** — matching decompilation work used as an accuracy reference.
- **pds-tools** — documentation and tooling for PDS assets, formats, music, video, fonts, and disc data.
- **Yabause / Vita Yabause** — Saturn hardware and behavioral reference where native behavior needs validation.

Lagi keeps Vita-specific code separated from upstream reconstruction work wherever practical so improvements can continue to be incorporated.

## Architecture

```text
Original Panzer Dragoon Saga data
              |
      Azel / ATOLM + pds-tools
              |
          Lagi runtime
              |
    SceGxm / SceCtrl / SceAudioOut
              |
        PlayStation Vita
```

The intended path is `PDS game logic -> native ARMv7 -> Vita platform layer -> Vita hardware`, rather than SH-2 emulation.

## Initial bring-up

Development is intentionally staged. The first target does **not** require graphics or audio.

1. Build/package a VitaSDK executable and VPK.
2. Initialize the Vita platform layer.
3. Link the portable/reconstructed game runtime.
4. Run with null renderer and null audio.
5. Locate user-supplied PDS data.
6. Reach game/runtime initialization on real Vita hardware.
7. Add Vita controller input.
8. Bring up native SceGxm rendering with reconstructed PDS geometry.
9. Translate PDS/VDP1 and VDP2 rendering behavior incrementally.
10. Add native audio and remaining platform systems.

## Planned platform mapping

| Runtime concept | Lagi / Vita target |
| --- | --- |
| Desktop entry/platform code | VitaSDK |
| SDL host services | Vita platform layer |
| BGFX rendering | Native SceGxm |
| Saturn/desktop input | SceCtrl |
| Desktop audio output | SceAudioOut |
| Standard file access | Vita filesystem / stdio |
| ImGui / desktop debug UI | Disabled initially; Vita diagnostics later |
| Tracy | Disabled on Vita |
| Saturn timing/VBlank behavior | Vita timing + compatibility layer |

## Game data

**No Panzer Dragoon Saga game data is included in this repository.**

Lagi is intended to operate using data supplied by the user from a legally obtained copy of *Panzer Dragoon Saga*. Current hardware builds read user-supplied Disc 1 BIN/CUE images directly and mount the ISO9660 filesystem at runtime.

Current Disc 1 layout:

```text
ux0:data/lagi/Disc 1/
    <disc>.cue
    <disc>.bin
```

The CUE file is parsed to locate the MODE1 data track and `COMMON.DAT` / `DRAGON0.MCB` are read directly from the image.

## Rendering strategy

Lagi will translate reconstructed game intent and Saturn render semantics directly to native SceGxm rather than emulate a complete Saturn graphics subsystem unless a particular behavior requires it. The renderer is intended to preserve Saturn-era characteristics such as low color precision, Gouraud behavior, mesh transparency, and VDP1/VDP2 compositing rather than silently modernize them. The deliberate visual exception is resolution: Lagi targets the Vita's native 960x544 output rather than reproducing the Saturn's lower render resolution.

```text
clear screen
 -> Basic Wing debug geometry
 -> PDS geometry
 -> textures
 -> vertex/Gouraud lighting
 -> VDP1-specific behavior
 -> VDP2/background layers
 -> priority and compositing
```

Correctness can be compared against original Saturn behavior and Vita Yabause during development.

## Building

A working **VitaSDK** installation is required.

```sh
mkdir build
cd build
cmake ..
make
```

For the current native GXM viewer, `psp2cgc` must also be available on `PATH` or via the `PSP2CGC` environment variable. See `docs/BUILDING.md` and `docs/STATUS.md` for the current hardware workflow and milestones.

## Legal

Lagi is an independent community reimplementation project. Sega, Panzer Dragoon, Panzer Dragoon Saga, and related names and assets belong to their respective owners.

This repository does not distribute copyrighted game data. Users are responsible for supplying required game files from copies they are legally entitled to use.

## Acknowledgements

Lagi exists because of years of Saturn and Panzer Dragoon reverse-engineering work by the Azel, ATOLM, pds-tools, Yabause, and wider Sega Saturn development communities.


### Graphics progress

The current Vita renderer can display the Basic Wing using the original Saturn VDP1 texture data and a deliberately Saturn-faithful lighting/output path. It reconstructs four-corner Gouraud shading across Saturn quads, performs the lighting in RGB555-style 5-bit color space, restores visible color banding before presenting through the Vita's RGBA8888 framebuffer, applies the hardware-validated face winding, and plays the dragon morph-screen flap animation at a fixed 30 Hz. The goal is not to modernize the visual output, but to preserve the rendering limitations that define Panzer Dragoon Saga's original look while executing natively on Vita hardware.

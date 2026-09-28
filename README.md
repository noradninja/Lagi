# Lagi

**Lagi** is a native PlayStation Vita reimplementation of *Panzer Dragoon Saga*, adapting reconstructed game logic and Saturn systems to VitaSDK and VitaGL.

The goal is not to emulate a Sega Saturn. Lagi aims to execute reconstructed PDS game logic natively on the Vita's ARM CPU while translating the game's platform, rendering, input, audio, and storage requirements to Vita hardware.

> **Status:** very early bring-up. The first milestone is a native Vita executable that can initialize the reconstructed game runtime with rendering and audio disabled.

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
   VitaGL / SceCtrl / SceAudioOut
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
8. Bring up VitaGL rendering with simple geometry.
9. Translate PDS/VDP1 and VDP2 rendering behavior incrementally.
10. Add native audio and remaining platform systems.

## Planned platform mapping

| Runtime concept | Lagi / Vita target |
| --- | --- |
| Desktop entry/platform code | VitaSDK |
| SDL host services | Vita platform layer |
| BGFX rendering | VitaGL |
| Saturn/desktop input | SceCtrl |
| Desktop audio output | SceAudioOut |
| Standard file access | Vita filesystem / stdio |
| ImGui / desktop debug UI | Disabled initially; Vita diagnostics later |
| Tracy | Disabled on Vita |
| Saturn timing/VBlank behavior | Vita timing + compatibility layer |

## Game data

**No Panzer Dragoon Saga game data is included in this repository.**

Lagi is intended to operate using data supplied by the user from a legally obtained copy of *Panzer Dragoon Saga*. Early development will use extracted files; direct disc-image access can come later.

Initial data path:

```text
ux0:data/lagi/
```

## Rendering strategy

Lagi will translate reconstructed game intent and already-processed render data to VitaGL as directly as practical rather than emulate a complete Saturn graphics subsystem unless a particular behavior requires it.

```text
clear screen
 -> primitive
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

Exact dependency setup will be expanded as the bootstrap becomes functional.

## Legal

Lagi is an independent community reimplementation project. Sega, Panzer Dragoon, Panzer Dragoon Saga, and related names and assets belong to their respective owners.

This repository does not distribute copyrighted game data. Users are responsible for supplying required game files from copies they are legally entitled to use.

## Acknowledgements

Lagi exists because of years of Saturn and Panzer Dragoon reverse-engineering work by the Azel, ATOLM, pds-tools, Yabause, and wider Sega Saturn development communities.

# Arachnoth device follow-up

Inputs: `2026-10-19 23-12-36.mp4` (334.5 seconds), fresh `lagi.log`, and
Lagi/Saturn cinematic screenshots. Speech is hardware-confirmed. The video
contains wireframe combat before the E014 cinematic; samples at 30, 60, 90 and
120 seconds show the attack-camera/world relationship. It is not a matched
Saturn animation comparison and does not justify disabling world depth.

## Devour: recovered stalled phases

`Arachnoth.cpp` phase 1 evaluates one unused scalar and immediately returns.
It never advances the common counter or reaches phase 2. Original BTL_A3_2
06057E94..06058076 instead computes all three approach axes, integrates the
camera spring with damping 0x1999, clears the accumulator and changes phase at
counter 0x19. The generated adapter restores those operations.

Phase 3 has a second permanent return at counter 0x4F. Original
06058268..060582FC computes both quadrant offsets, installs current/desired
camera pointers, resets interpolation, and reaches the common increment.
That close-up is restored as well. The original frame-0x71 damage call remains.
No damage amount, animation speed or attack selection is invented. Other
reconstructed phases, model/jaw callbacks and the complete attack still need
hardware comparison; this is not a claim of complete Devour visual fidelity.

The production helper fixture checks all axes, damping, counter-25 phase change
and four close-up quadrants. The adapter fails configuration if either old stub
changes. Both Vita build configurations compile; on-device attack completion,
damage/release and animation appearance remain acceptance items.

## Still open

### 2026-10-20: Charge anchor correction

The two reconstructed `m240` return anchors were positive Z (0x1D2000 and
0x230000). Original BTL_A3_2 data at 060A909C and 060A9090 has negative Z
(-0x1D2000 and -0x230000), with X=0x201000/Y=0x13000. Initial placement already
uses the correct negative side. The generated adapter now decodes both points
through `readSaturnVec3`, rather than maintaining transcribed coordinates.
Stable host storage preserves the pointers' lifetime. The direct-disc and
generated-source fixture verifies both points. Charge/recovery timing and
acceleration coefficients remain unchanged; device movement is not yet accepted.

- Charge: native tail 06059FB8..0605A0F8 really uses signed -0x28F then +0xA3
  on the same target/position delta. Both are already in the reconstruction.
  Flipping that sign to hide wall intersection would not be source-faithful.
  Attack initialization, coordinate spaces and remaining phases need comparison.
- Cinematic light: hardware still differs despite RGB transport tests. E014's
  log reports RGB 0E1316 with falloffs 010202/EAEAEA/E0E0E0. This proves setup
  data arrived, not correct normals, camera/model transforms, palette, floor
  texture or final light evaluation. No screenshot-matched tint was added.
- System sounds: the log does contain SCSP starts for menu/target sound IDs
  0/1/2/3 and native bank/control mappings. Enqueue/start logs are not audible
  acceptance; shared bank/sample/mixer behavior still needs tracing.
- World map: source case 10 requests 0x4F, and original MAIN 06027C72..74 also
  calls 06027110 with 0x4F. This is not merely a Vita adapter typo. The current
  dispatch interpretation was wrong: subsequent inspection proves native mode
  6 loads WORLD.PRG and mode 5 loads CHANGE.PRG. See WORLD_MAP_HANDOFF.md for
  the corrected, explicitly non-playable handoff boundary and remaining ports.

Music/output recovery is documented separately in `CINEMATIC_PCM_AUDIO.md`.

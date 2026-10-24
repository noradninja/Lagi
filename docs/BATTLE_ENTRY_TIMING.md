# Battle-entry timing investigation

## Hardware report: 191f844

The user corrected the tested revision to **191f844**. The Downloads log
written 2026-10-24 01:28 contains six completed random encounters (sub IDs
7, 0, 6, 7, 0, 6). The user reports smooth 30 FPS after loading, including
multiple instances of the same enemy. This does not validate the later
8fc1bea CPU-storage optimization; be4fd3b only adds cadence tests.

First-publication renderer outliers at those entries are 73.153, 69.042,
68.790, 71.598, 69.244 and 65.316 ms. RBG preparation contributes
26.239, 26.277, 26.180, 26.561, 26.046 and 25.680 ms respectively.
These are overlapping component timings, not independent costs to sum.
The log has no scene-fade RGB on these frames, so it cannot prove whether
the expensive publication was entirely hidden by white.

## Current reconstructed source sequence

- `battleOverlayTask_Init` starts both fades toward white over 0x3C.
- Overlay draw case 0 sets graphicEngineStatus.m4; case 1 starts the
  authored overlay and font loading. There is no explicit fade-stopped
  check in this function. Do not assume every load already waits for white.
- `battleMainTask_Draw` runs music setup, waits for sound readiness, then
  resets the engine/camera/grid and invokes battle resource/engine setup.
- `battleEngine_Init` initializes environment/actors and sets fade0 to
  white. Battle intro submode 1 waits several updates, then requests the
  0x5A white-to-neutral fade and starts the intro task.

This is a source audit of the current reconstruction, not proof of original
Saturn instruction ordering. Do not add an arbitrary delay or advance a
second fade clock on that basis. Preserve native decisions and investigate
the actual publication boundary first.

## Diagnostic addition

Existing FlightPresentOutlier records now append fadeR/fadeG/fadeB from
the renderer's published g_sceneColorOffset, the same values used by scene
composition. Positive values clamp to 255 in the additive pass: all three
at least 255 imply full white for that pass. This is not a measurement of
every earlier CPU load or proof of display scanout timing.

Existing bounded RBG0PageCache records append decodeUs (page validation and
decode combined) and atlasCopyUs (scratch-to-CDRAM row copies). They retain
the existing log budget and heartbeat; no additional log lines or effects
are introduced. Timers add two clock reads per required page and two per
rebuilt page. Total prepUs still includes other service work and allocation.

Host page-service tests exercise the actual production include, including
mock-clock attribution on two rebuilt pages. Native pixel-oracle tests
check output. Combat profiling tests check that the logged fade values
match the composition source. None proves a speed improvement or Vita
transition smoothness. Goal 1 remains active pending hardware evidence.

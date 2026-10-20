# Shared cinematic PCM service

Hardware has accepted E014 playback after Arachnoth. World-map bring-up is
separate. This change restores the previously skipped EPK audio packets; it
does not change cinematic scripts, assets, frame counts or encounter decisions.

## Original packet contract

MAIN `0600ED20` handles opcode 1: a 16-byte header precedes signed PCM payload.
It halves the payload span for two-channel input and copies left/right planes
separately (`0600ED88..0600EDD4`). Metadata supplies channel count, 8/16-bit depth,
and 16.16 sample rate. PCM16 is big-endian; PCM8 is signed. The Vita adapter
handles payload and header ring-wrap without retaining command-ring pointers.

Original-disc tests inventory every top-level packet and round-trip all PCM
samples through the production decoder:

- E011: 238 audio packets, 669,078 mono PCM16 frames at 22,050 Hz.
- E014: 977 audio packets, 2,696,790 mono PCM16 frames at 22,050 Hz.
- E006.EPK: no embedded audio packets. This service cannot supply audio absent
  from that file; separate standalone field/voice PCM loading remains unfinished.

Audio duration is not cinematic duration: voice packets occur among timed
animation/camera packets, with silence between speech sections.

## Service boundary and lifecycle

The existing SCSP worker mixes decoded PCM additively into its stereo output,
with signed saturation and linear sample-rate conversion to 44,100 Hz. It does
not open a competing output port, stop BGM, add another DSP worker or replace
authored sounds. The PCM lane is dry output; this is not a claim to emulate any
SCSP effects routing specific to streamed voices.

A fixed 65,536-frame stereo SPSC ring uses 32-bit atomic cursors and a reset
generation handshake. Producer data is copied only when the whole packet fits;
busy returns leave Azel's packet at its read cursor for retry. Completion/skip
cancels that stream's generation; pause retains queued samples without consuming
them. The audio worker acknowledges reset before accepting replacement data,
preventing old cutscene packets from leaking into the next scene. No per-packet
heap allocation or game-thread audio-output wait is introduced.

Bounded diagnostics: `[EPKAudio] start`, first two accepted packets, and `stop`
with packet count/remaining queued frames. Invalid formats are logged and close
the stream rather than causing an infinite retry. Existing Azel presentation
timing remains in use; device lip sync/latency must be checked, not inferred from
matching sample counts. Final queued samples are cancelled on native closure.

## Validation and acceptance

### Fade-all / first-packet stall correction

The first device test stopped E011 command consumption at frame 638: the audio
start was logged but no packet was accepted. Fade-all disables gameplay output;
the worker previously acknowledged PCM generations only inside that disabled
render path. The worker now services generation changes regardless of device
queue capacity, and routes active EPK PCM independently of gameplay synthesis.
When gameplay is muted, its driver still clocks with discarded output while EPK
samples mix into a cleared buffer. Output queue pacing replaces the FMV discard
sleep in this lane. This does not resume old gameplay music or take a second
audio port. Queue tests cover acknowledgement without rendering and stop state;
device audio and cinematic completion still require a fresh run.

Host tests cover all signed formats, planar stereo, odd-byte ring wrap, additive
saturation, 22,050-to-44,100 interpolation, whole-packet backpressure, generation
reset/stop, pause and 10,000-frame concurrent SPSC transfer. A second fixture runs
the actual generated packet handler for accepted, busy, wrapped and invalid
requests. Original-disc decoder tests contain no copied game data in the repo.

Both full-resolution/dithered Vita build variants compile. These are source,
build and host/data checks, not hardware listening acceptance. On the next run,
check E011 and E014 speech, timing, BGM/SFX coexistence and clean scene exit; send
the fresh log. OBS captures with a silent audio track cannot validate listening.

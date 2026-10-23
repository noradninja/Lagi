# ISO loader acceptance and next-thread handoff

The four-disc loader is finished. On 2026-10-23 the user tested it on Vita at
960x544 (`LAGI_FULLRES=ON`) and accepted the whole path: the screen and
medallions, disc detection, D-pad/left-stick selection, X to load, BGM, both
sound effects, and the fade into the game. The screenshot in the README is the
accepted on-device result. This is device feedback, not a conclusion drawn
from a desktop build.

Work lives on `feature/first-section-cleanup` in `noradninja/Lagi`. The branch
remains the working branch after its completed loader work is merged into
`main`; there is no need to restart the frontend design process in the next
thread. The two audio commits just before this handoff are `81234ca` (PCM
assets) and `f5940a4` (playback). The documentation/image update comes after
those commits. No visual assets were changed in the audio or handoff work.

At startup, Lagi checks `ux0:data/lagi/Disc 1` through `Disc 4` for supported
MODE1 CUE/BIN data tracks and probes their ISO9660 volumes. Missing discs stay
visible but cannot be selected. A valid X press mounts the selected image,
then the 30-vblank fade hands off to Azel's normal initialization. The Vita
frontend loads its approved image/font/audio assets from `app0:frontend/` and
uses the existing GXM/Neptune presentation. Azel still decides game state,
Lagi supplies platform services, and Neptune renders.

The navigation and confirm cues are 44.1-kHz stereo PCM rendered from the
user's original Disc 1 banks through Lagi's Saturn 68K/SCSP core. They are
the sounds selected by Azel's original title code: system IDs 10 and 0.
Navigation was independently matched to the supplied recording (0.956
aligned waveform correlation); the recording itself was never used as an
asset. The frontend mixes the cues into its music stream with a bounded
4096-frame input queue so they respond promptly. The music stream is released
after the fade and before native Azel audio starts.

Local checks passed before the Vita acceptance: a full-resolution Vita VPK
build, byte-for-byte verification of both packaged WAVs, and the COMMON
system-sound-bank descriptor test. The user has now supplied the missing
hardware acceptance. The build output is not a new public release.

For the next work thread, continue from the existing feature branch. The
first-section goals are tracked in [FIRST_SECTION_CLEANUP.md](FIRST_SECTION_CLEANUP.md);
the loader is not one of its remaining open items. In particular, do not treat
the accepted selector as evidence that the separate combat-performance,
rewards, field-map, or scene-coverage goals are complete. Preserve the
frontend and the established ownership boundaries while working on those.

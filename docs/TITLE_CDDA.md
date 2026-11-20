# Mounted-disc title CD audio

The title now requests AUDIO track 3 from the selected disc's CUE after its
native title sound-bank load completes. This restores the omitted request
beside the original fileInfoStruct track/repeat comments in o_title.cpp.
Generated-source adapters supply the request and titleScreenDrawSub3 stop
service; extern/Azel is unchanged. The accepted disc-selector MP3 and PCM
cues are separate and unchanged.

The disc service retains the selected CUE identity across availability and
bounded file probes. AUDIO/BINARY track indices are interpreted as stored
2352-byte sectors relative to their FILE, including split-file layouts.
Playback begins at INDEX 01; a following same-file INDEX 00 bounds playback
before the next stored pregap (otherwise its INDEX 01 bounds it). A separate
PREGAP directive adds no bytes. The final track ends at its backing file's
end. Bounds are checked; unsupported WAVE CUE files and absent/malformed
tracks fail silently apart from a CDDA diagnostic. No replacement music is
bundled. A data-only image cannot provide omitted CD audio.

One 37,632-byte cache streams little-endian 16-bit stereo at 44,100 Hz,
looping within the CUE range. The existing SCSP worker adds the CD samples
to its PCM output with signed saturation, retaining menu SFX. It acquires
no extra output port or worker, and respects the existing Cinepak output
lease. All CD file reads and closes belong to the audio worker. Title exit
and attract-mode stop requests ramp down over 22,050 samples; the battle
reset request and worker shutdown stop immediately. This half-second audio
ramp is the platform transition policy, not a claim that all original CD
controller fade timing has been independently instruction-audited.

Validation:

- tests/cdda_test.ps1 executes the production CUE parser and stream mixer:
  shared/split files, synthetic/stored pregaps, invalid/missing tracks,
  byte ordering, stereo, clipping, loop boundaries, fade, stop and short files.
- Original Disc 1 AZEL_1.cue/bin gives track 3 bytes
  648063024..664233024 (4,042,500 stereo frames). The production reader
  successfully reads the bounded first sector. No disc PCM is committed.
- Full-resolution Vita battle build/package passes.
- Actual Vita listening, other-disc music, seamless loops, SFX balance and
  title-to-load/new-game/attract handoff remain device acceptance gates.

Expected log: `[CDDA] disc=N track=3 ready=1 start=... end=... file=...`,
then `[CDDA] stop fadeFrames=...` on leaving the title. A ready=0 entry
requires checking the installed CUE and its referenced audio BIN(s).

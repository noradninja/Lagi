# Audio diagnostics opt-in

Venezia work is deferred on feature/venezia-audio-offload. Main retains the hardware-validated ARM DSP/SCSP implementation through 1e84d7e.

LAGI_AUDIO_DIAGNOSTICS defaults OFF. This disables routine and error audio trace output in the native audio service, PCM output, frontend music and DSP platform; worker and sample profiling, peak/slot sampling, DSP image trace hashes, MPRO rejection reports and LDSP capture file I/O. Enable with -DLAGI_AUDIO_DIAGNOSTICS=ON when investigating audio. Existing capture corpus and backend config remain usable. Global fatal logging and graphics profiling are independent.

Do not gate actual bank loading/residency verification, DSP reselection or VM publication, PCM submission and played-frame accounting, command delivery, clocks, scheduling or game behavior. The MPRO write hook still invalidates changed instructions. No renderer settings changed.

For FPS tests, begin with audio diagnostics OFF, optimized release settings and normal rendering. Existing ScenePerf, presentation cadence and deadline outlier logging remain available; serialized GPU pass timing stays OFF initially because it changes synchronization. Compare representative slow areas separately, using frame cadence and sampled stage timings rather than adding overlapping task/renderer measurements. Verify audio behavior and graphics performance on device; source changes alone are not hardware validation.

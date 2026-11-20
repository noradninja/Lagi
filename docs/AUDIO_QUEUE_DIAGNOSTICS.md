# SCSP queue and sound-command timing

**20 November 2026.** This checkpoint measures audio starvation rather than changing it. The M68K clock, SCSP/DSP processing, native ARM backend, queue target (2,048 frames), output block (1,024 frames), 256-frame synthesis quantum and thread scheduling are unchanged.

Our October 6 Vita baseline for an 84-step program had a 5,297-microsecond median processing time per quantum against a 5,804-microsecond deadline. The ARM DSP median was 2,268 microseconds. That headroom is small enough that occasional scheduling stalls could matter; the older measurements alone cannot establish whether the hardware output starved.

## New log fields

The worker records an `[AzelAudioQueue]` line at approximately one-second intervals. The following are **reporting-window measurements**, not emulation controls.

| Field | Meaning |
| --- | --- |
| `quanta`, `renderAvgUs`, `renderMaxUs`, `budgetUs` | Production chunks, average and worst observed synthesis/mix/enqueue cost and the nominal 5,804-us budget |
| `producerMin`, `producerMax` | Ring frames waiting at the start of an SCSP render quantum |
| `outputBlocks`, `outputMin`, `outputMax` | Hardware PCM submissions and ring frames available before each output block |
| `underrunBlocks`, `emptyBlocks`, `zeroFillFrames`, `maxMissing` | Output submissions with missing source audio, completely empty blocks, total padded silent frames and worst block deficit |
| `events`, `eventAvgUs`, `eventMaxUs` | Sound request to worker event dequeue time |
| `dispatches`, `dispatchAvgUs`, `dispatchMaxUs` | Sound request to the native sound-dispatch **attempt**, including any bank-loading hold |
| `acks`, `ackAvgUs`, `ackMaxUs`, `mailboxPendingMs` | Time from the M68K mailbox request flag to its observed clearance; pending age if not yet acknowledged |

The output counters exclude the deliberate startup prebuffer wait and an intentional end-of-stream partial block. The output thread only updates atomics; it never writes logs or takes timestamps. The SCSP worker performs two additional timer reads per 256-frame quantum and reports from its control-loop boundary. Cross-thread watermarks are approximate interval observations. We have not measured the instrumentation's overhead on Vita yet.

The `dispatch` metric is not proof the driver accepted that individual sound, and the `ack` metric covers a *mailbox transaction*, which can contain several commands. Neither is the time until a sound is audible. They are enough to distinguish event queue waits, native loading holds, and driver acknowledgement stalls before deeper tracing.

## Hardware capture

Use a build from `diagnostics/audio-queue-starvation` with the normal native A3 and ARM DSP settings. Keep the Vita clocks, scene and build options fixed. Record a steady music section, a busy battle, a bank/scene transition and a few menu/LCS sound effects. Save the matching `ux0:data/lagi/lagi.log` and any video.

Compare `zeroFillFrames` and `underrunBlocks` with `renderMaxUs`, then look at the existing `[AzelAudioPerf]` M68K/slot/DSP breakdown. A larger buffer could hide short spikes, but cannot resolve synthesis that is consistently slower than real time. A large `dispatchMaxUs` without a large `eventMaxUs` would point toward the native loading path rather than PCM buffering.

The Windows fixture `tests/audio_output_telemetry_test.ps1` covers full, partial and empty blocks, interval counters and reset. A passing host test is not a hardware performance result.

The first commit on this branch adds output-starvation counters and the host fixture. The second adds reporting, render quantum timing and staged sound-command latency. Both commits leave playback behavior unchanged.

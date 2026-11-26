#ifndef LAGI_AUDIO_DIAGNOSTICS_H
#define LAGI_AUDIO_DIAGNOSTICS_H
/* Observational instrumentation only. Never put playback side effects
 * inside these macros or diagnostic-only branches. */
#ifndef LAGI_AUDIO_DIAGNOSTICS
#define LAGI_AUDIO_DIAGNOSTICS 0
#endif
#if LAGI_AUDIO_DIAGNOSTICS
#define LAGI_AUDIO_LOG(...) ::lagi::platform::logging::writef(__VA_ARGS__)
#define LAGI_DSP_LOG(...) lagi_dsp_log(__VA_ARGS__)
#else
#define LAGI_AUDIO_LOG(...) ((void)0)
#define LAGI_DSP_LOG(...) ((void)0)
#endif
#endif

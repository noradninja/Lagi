#ifndef LAGI_DSP_PLATFORM_H
#define LAGI_DSP_PLATFORM_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
struct _SCSPDSP;
void lagi_dsp_program_written(struct _SCSPDSP *DSP);
void lagi_dsp_service_pending(struct _SCSPDSP *DSP);
void lagi_dsp_release_native(void);
enum { LAGI_DSP_PREDECODED, LAGI_DSP_REFERENCE, LAGI_DSP_AUTO, LAGI_DSP_AOT, LAGI_DSP_ARM };
#ifdef LAGI_DSP_TEST
static inline int lagi_dsp_platform_init(void) { return LAGI_DSP_PREDECODED; }
static int g_lagiDspTestVm, g_lagiDspTestPublishFailure;
static uint32_t g_lagiDspTestCode[4][16384];
static inline int lagi_dsp_vm_available(void) { return g_lagiDspTestVm; }
static inline void lagi_dsp_capture(const uint16_t *words, unsigned steps, uint32_t hash) { (void)words; (void)steps; (void)hash; }
static inline void lagi_dsp_log(const char *format, ...) { (void)format; }
static inline void *lagi_dsp_vm_publish(unsigned slot, const uint32_t *code, unsigned bytes) { unsigned i; if (!g_lagiDspTestVm || g_lagiDspTestPublishFailure || slot>=4 || bytes>65536) return 0; for(i=0;i<bytes/4;++i) g_lagiDspTestCode[slot][i]=code[i]; return g_lagiDspTestCode[slot]; }
#else
int lagi_dsp_platform_init(void);
int lagi_dsp_vm_available(void);
void lagi_dsp_capture(const uint16_t *words, unsigned steps, uint32_t hash);
void lagi_dsp_log(const char *format, ...);
void *lagi_dsp_vm_publish(unsigned slot, const uint32_t *code, unsigned bytes);
void lagi_dsp_platform_shutdown(void);
#endif
static inline uint32_t lagi_dsp_program_hash(const uint16_t *words, unsigned steps) {
    uint32_t hash = 2166136261u;
    unsigned i;
    hash = (hash ^ steps) * 16777619u;
    for (i=0; i<steps*4; ++i) {
        hash = (hash ^ (words[i] & 255u)) * 16777619u;
        hash = (hash ^ (words[i] >> 8)) * 16777619u;
    }
    return hash;
}
#ifdef __cplusplus
}
#endif
#endif

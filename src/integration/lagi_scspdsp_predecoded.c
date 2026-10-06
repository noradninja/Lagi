#include <assert.h>
#include <string.h>

#include <psp2/kernel/threadmgr.h>

#include "ao.h"
#include "cpuintrf.h"
#include "scsp.h"
#include "lagi_dsp_platform.h"

/*
 * Lagi-owned drop-in replacement for AOSDK scspdsp.c.
 *
 * The Saturn SCSP DSP microprogram is static between SCSPDSP_Start() calls,
 * but the original interpreter decodes all MPRO bitfields on every 44.1 kHz
 * sample. PDS Ruins uses 84 DSP steps, so that repeated decode work is
 * substantial on Vita.
 *
 * This implementation preserves the original arithmetic, memory accesses,
 * precision and execution order. It only predecodes invariant instruction
 * control fields when SCSPDSP_Start() is called.
 */

#include "lagi_dsp_ir.h"

typedef struct LagiPdsFastOp {
    const INT32* input;
    const INT16* coef;
    const UINT16* masa;
    UINT8 TRA;
    UINT8 TWA;
    UINT8 IWA;
    UINT8 EWA;
    UINT8 flags0;
    UINT8 flags1;
    UINT8 inputShift;
    UINT8 addrIncrement;
} LagiPdsFastOp;

enum {
    LAGI_PDS_TWT   = 1u << 0,
    LAGI_PDS_XSEL  = 1u << 1,
    LAGI_PDS_IWT   = 1u << 2,
    LAGI_PDS_MWT   = 1u << 3,
    LAGI_PDS_MRD   = 1u << 4,
    LAGI_PDS_EWT   = 1u << 5,
    LAGI_PDS_TABLE = 1u << 6
};

enum {
    LAGI_PDS_SHIFT1   = 1u << 0,
    LAGI_PDS_YCOEF    = 1u << 1,
    LAGI_PDS_NEGB     = 1u << 2,
    LAGI_PDS_ZERO     = 1u << 3,
    LAGI_PDS_BSEL     = 1u << 4,
    LAGI_PDS_IWT_ALIAS = 1u << 5,
    LAGI_PDS_TEMP_USED = 1u << 6,
    LAGI_PDS_SHIFT_USED = 1u << 7
};

static const INT32 g_lagiPdsZeroInput = 0;
static LagiScspDspOp g_lagiDspOps[128];
static LagiPdsFastOp g_lagiPdsFastOps[128];
static struct _SCSPDSP* g_lagiDecodedDsp = 0;
static int g_lagiDecodedSteps = 0;
static int g_lagiPdsFastPath = 0;

volatile unsigned lagi_dsp_mix_serial = 0;
volatile unsigned lagi_dsp_fast_path = 0;
volatile unsigned lagi_dsp_mix_steps = 0;
volatile unsigned lagi_dsp_mix_iwt = 0;
volatile unsigned lagi_dsp_mix_twt = 0;
volatile unsigned lagi_dsp_mix_mrd = 0;
volatile unsigned lagi_dsp_mix_mwt = 0;
volatile unsigned lagi_dsp_mix_ewt = 0;
volatile unsigned lagi_dsp_mix_adrl = 0;
volatile unsigned lagi_dsp_mix_frcl = 0;
volatile unsigned lagi_dsp_mix_yrl = 0;
volatile unsigned lagi_dsp_mix_xinput = 0;
volatile unsigned lagi_dsp_mix_yfrc = 0;
volatile unsigned lagi_dsp_mix_ycoef = 0;
volatile unsigned lagi_dsp_mix_yreg = 0;
volatile unsigned lagi_dsp_mix_satshift = 0;
volatile unsigned lagi_dsp_mix_wrapshift = 0;
volatile unsigned lagi_dsp_mix_unpack = 0;
volatile unsigned lagi_dsp_mix_pack = 0;
volatile unsigned lagi_dsp_mix_noflr = 0;
volatile unsigned lagi_dsp_mix_noflw = 0;
volatile unsigned lagi_dsp_profile_sample = 0;
volatile unsigned long long lagi_dsp_profile_last_us = 0;

static INT32 g_lagiUnpackTable[65536];
static int g_lagiUnpackTableReady = 0;

static UINT16 PACK(INT32 val)
{
    const UINT32 raw = (UINT32)val;
    const UINT32 sign = (raw >> 23) & 0x1u;
    const UINT32 temp = (raw ^ (raw << 1)) & 0x00FFFFFFu;
    UINT32 exponent;
    UINT32 mantissa;

    /*
     * Original AOSDK scans bit 23 leftward for at most 12 iterations.
     * CLZ gives the identical exponent in one ARM instruction. Keep the CLZ
     * path, but perform all shifts in unsigned arithmetic so negative 24-bit
     * DSP values cannot invoke signed-left-shift undefined behavior.
     */
    if (temp == 0u)
        exponent = 12u;
    else
    {
        exponent = (UINT32)__builtin_clz(temp) - 8u;
        if (exponent > 12u)
            exponent = 12u;
    }

    if (exponent < 12u)
        mantissa = ((raw << exponent) & 0x003FFFFFu) >> 11;
    else
        mantissa = raw & 0x7FFu;

    return (UINT16)(
        mantissa |
        (sign << 15) |
        (exponent << 11));
}

static INT32 lagi_sign_extend24_u32(UINT32 v)
{
    v &= 0x00FFFFFFu;
    if (v & 0x00800000u)
        v |= 0xFF000000u;
    return (INT32)v;
}

static INT32 lagi_unpack_scalar(UINT16 val)
{
    const UINT32 sign = ((UINT32)val >> 15) & 0x1u;
    UINT32 exponent = ((UINT32)val >> 11) & 0xFu;
    const UINT32 mantissa = (UINT32)val & 0x7FFu;
    UINT32 uval = mantissa << 11;

    if (exponent > 11u)
    {
        exponent = 11u;
        uval |= sign << 22;
    }
    else
    {
        uval |= (sign ^ 1u) << 22;
    }

    uval |= sign << 23;

    /*
     * The original AOSDK code sign-extends bit 23 via signed << / >>.
     * Do that explicitly in defined unsigned arithmetic, then perform the
     * arithmetic right shift required by the SCSP packed format.
     */
    {
        const INT32 signed24 = lagi_sign_extend24_u32(uval);
        return signed24 >> exponent;
    }
}

static void lagi_init_unpack_table(void)
{
    unsigned i;
    if (g_lagiUnpackTableReady)
        return;

    for (i = 0; i < 65536u; ++i)
        g_lagiUnpackTable[i] = lagi_unpack_scalar((UINT16)i);

    g_lagiUnpackTableReady = 1;
}

static inline INT32 UNPACK(UINT16 val)
{
    return g_lagiUnpackTable[val];
}

static void lagi_scspdsp_decode(struct _SCSPDSP* DSP)
{
    int step;

    g_lagiDecodedDsp = DSP;
    g_lagiDecodedSteps = DSP->LastStep;

    /* Both native backends and the reference share one field schema. */
    for (step = 0; step < DSP->LastStep; ++step) {
        const UINT16 *p = DSP->MPRO + step*4;
        LagiScspDspOp *op = &g_lagiDspOps[step];
#define LAGI_DSP_FIELD(name, word, shift, mask) op->name=(p[word] >> shift)&mask;
#include "lagi_dsp_fields.def"
#undef LAGI_DSP_FIELD
        op->odd=step&1;
    }

    /*
     * PDS DSP banks observed so far share a much narrower instruction subset:
     * - Y source is only FRC or COEF
     * - FRCL/YRL/ADRL are never used, so FRC/Y/ADRS registers stay zero
     * - SHIFT is only saturating mode 0 or 1
     * - SCSP RAM traffic always uses PACK/UNPACK (NOFL=0)
     *
     * Build a compact second micro-op stream only when the entire program
     * satisfies those invariants. Unknown banks retain the generic interpreter.
     */
    g_lagiPdsFastPath = 1;
    for (step = 0; step < DSP->LastStep; ++step)
    {
        const LagiScspDspOp* op = &g_lagiDspOps[step];
        LagiPdsFastOp* fast = &g_lagiPdsFastOps[step];
        UINT8 f0 = 0;
        UINT8 f1 = 0;

        if (op->YSEL > 1 || op->FRCL || op->YRL || op->ADRL ||
            op->SHIFT > 1 || op->NOFL)
        {
            g_lagiPdsFastPath = 0;
            break;
        }

        if (op->IRA <= 0x1F)
        {
            fast->input = &DSP->MEMS[op->IRA];
            fast->inputShift = 0;
        }
        else if (op->IRA <= 0x2F)
        {
            fast->input = &DSP->MIXS[op->IRA - 0x20];
            fast->inputShift = 4;
        }
        else
        {
            fast->input = &g_lagiPdsZeroInput;
            fast->inputShift = 0;
        }

        fast->coef = &DSP->COEF[op->COEF];
        fast->masa = &DSP->MADRS[op->MASA];
        fast->TRA = op->TRA;
        fast->TWA = op->TWA;
        fast->IWA = op->IWA;
        fast->EWA = op->EWA;
        fast->addrIncrement = op->NXADR;

        if (op->TWT)   f0 |= LAGI_PDS_TWT;
        if (op->XSEL)  f0 |= LAGI_PDS_XSEL;
        if (op->IWT)   f0 |= LAGI_PDS_IWT;
        if (op->odd && op->MWT) f0 |= LAGI_PDS_MWT;
        if (op->odd && op->MRD) f0 |= LAGI_PDS_MRD;
        if (op->EWT)   f0 |= LAGI_PDS_EWT;
        if (op->TABLE) f0 |= LAGI_PDS_TABLE;

        if (op->SHIFT)    f1 |= LAGI_PDS_SHIFT1;
        if (op->YSEL)     f1 |= LAGI_PDS_YCOEF;
        if (op->NEGB)     f1 |= LAGI_PDS_NEGB;
        if (op->ZERO)     f1 |= LAGI_PDS_ZERO;
        if (op->BSEL)     f1 |= LAGI_PDS_BSEL;
        if (op->IWT && op->IRA == op->IWA)
            f1 |= LAGI_PDS_IWT_ALIAS;
        if ((!op->ZERO && !op->BSEL) || !op->XSEL)
            f1 |= LAGI_PDS_TEMP_USED;

        /* Only these side effects consume the previous ACC's SHIFTED value.
         * FRCL/ADRL are excluded by the guard above; even-step MWT is inert. */
        if (f0 & (LAGI_PDS_TWT | LAGI_PDS_MWT | LAGI_PDS_EWT))
            f1 |= LAGI_PDS_SHIFT_USED;

        fast->flags0 = f0;
        fast->flags1 = f1;
    }
    lagi_dsp_fast_path = (unsigned)g_lagiPdsFastPath;
}

typedef void (*LagiDspNativeFn)(struct _SCSPDSP *);
typedef struct LagiDspAotEntry {
    unsigned steps;
    UINT32 hash;
    UINT16 words[512];
    LagiDspNativeFn function;
    const char *codeBegin, *codeEnd;
} LagiDspAotEntry;
static INT32 lagi_sign_extend13_u32(UINT32 v);
static INT32 lagi_sat24(INT64 v);
static inline INT32 lagi_dsp_native_sat(INT32 acc,unsigned twice);
#include "lagi_dsp_aot.inc"
#include "lagi_dsp_arm.inc"

typedef struct LagiDspCacheEntry {
    unsigned steps, bytes;
    UINT32 hash;
    UINT16 words[512];
    LagiDspNativeFn function;
} LagiDspCacheEntry;
static LagiDspCacheEntry g_lagiNativeCache[4];
static LagiDspProgram g_lagiTranslation;
static UINT32 g_lagiArmScratch[16384];
static unsigned g_lagiCacheNext, g_lagiCacheHits, g_lagiCacheMisses;
static struct _SCSPDSP *g_lagiPendingDsp;
static struct { UINT16 words[512]; unsigned steps; UINT32 hash; } g_lagiCaptureQueue[32];
static unsigned g_lagiCaptureRead, g_lagiCaptureWrite, g_lagiCaptureDrops;
static void lagi_dsp_queue_capture(struct _SCSPDSP *DSP,UINT32 hash) {
    unsigned i;
    const unsigned steps=(unsigned)DSP->LastStep;
    for(i=g_lagiCaptureRead;i<g_lagiCaptureWrite;++i) {
        const unsigned slot=i%32;
        if(g_lagiCaptureQueue[slot].hash==hash && g_lagiCaptureQueue[slot].steps==steps &&
            !memcmp(g_lagiCaptureQueue[slot].words,DSP->MPRO,steps*8)) return;
    }
    if(g_lagiCaptureWrite-g_lagiCaptureRead==32) {
        ++g_lagiCaptureDrops;
        lagi_dsp_log("[LagiDSPCapture] reason=queue-full drops=%u\n",g_lagiCaptureDrops);
        return;
    }
    i=g_lagiCaptureWrite++%32;
    g_lagiCaptureQueue[i].steps=steps; g_lagiCaptureQueue[i].hash=hash;
    memcpy(g_lagiCaptureQueue[i].words,DSP->MPRO,steps*8);
}
static void lagi_dsp_capture_one(void) {
    if(g_lagiCaptureRead!=g_lagiCaptureWrite) {
        const unsigned slot=g_lagiCaptureRead++%32;
        lagi_dsp_capture(g_lagiCaptureQueue[slot].words,g_lagiCaptureQueue[slot].steps,g_lagiCaptureQueue[slot].hash);
    }
}
static LagiDspNativeFn g_lagiNativeFunction;
static struct _SCSPDSP *g_lagiNativeDsp;
static int g_lagiBackendMode=LAGI_DSP_PREDECODED;
static struct { struct _SCSPDSP *dsp; int dirty; } g_lagiDirtyInstances[8];
static int g_lagiDirtyOverflow;
static unsigned g_lagiNativeBackend;
volatile unsigned lagi_dsp_selected_backend=1;
volatile unsigned lagi_dsp_selected_hash;
static int lagi_dsp_is_dirty(struct _SCSPDSP *DSP) {
    unsigned i;
    if (g_lagiDirtyOverflow) return 1;
    for(i=0;i<8;++i) if(g_lagiDirtyInstances[i].dsp==DSP) return g_lagiDirtyInstances[i].dirty;
    return 0;
}
static void lagi_dsp_clear_dirty(struct _SCSPDSP *DSP, int release) {
    unsigned i;
    for(i=0;i<8;++i) if(g_lagiDirtyInstances[i].dsp==DSP) {
        g_lagiDirtyInstances[i].dirty=0;
        if(release) g_lagiDirtyInstances[i].dsp=0;
    }
}

/* All MPRO writes from the generated SCSP service arrive on the audio worker.
 * Dirty programs use the generic reference until Start selects a replacement. */
void lagi_dsp_program_written(struct _SCSPDSP *DSP) {
    unsigned i, empty=8;
    for(i=0;i<8;++i) {
        if(g_lagiDirtyInstances[i].dsp==DSP) { g_lagiDirtyInstances[i].dirty=1; break; }
        if(!g_lagiDirtyInstances[i].dsp) empty=i;
    }
    if(i==8) {
        if(empty<8) { g_lagiDirtyInstances[empty].dsp=DSP; g_lagiDirtyInstances[empty].dirty=1; }
        else { g_lagiDirtyOverflow=1; lagi_dsp_log("[LagiDSP] reason=instance-capacity backend=reference\n"); }
    }
    if(g_lagiPendingDsp==DSP) g_lagiPendingDsp=0;
    if(g_lagiDecodedDsp==DSP) g_lagiDecodedSteps=-1;
    if(g_lagiNativeDsp==DSP || g_lagiDirtyOverflow) g_lagiNativeFunction=0;
    lagi_dsp_selected_backend=0;
}
static void lagi_dsp_select_native(struct _SCSPDSP *DSP,int allowCompile) {
    const unsigned long long started=sceKernelGetSystemTimeWide();
    const unsigned steps=(unsigned)DSP->LastStep;
    const UINT32 hash=lagi_dsp_program_hash(DSP->MPRO,steps);
    const char *backend=g_lagiPdsFastPath?"predecoded":"reference";
    const char *reason="configured-predecoded";
    unsigned bytes=0, i;
    if(g_lagiPendingDsp==DSP) g_lagiPendingDsp=0;
    g_lagiNativeDsp=DSP; g_lagiNativeFunction=0; lagi_dsp_clear_dirty(DSP,0);
    if (g_lagiDirtyOverflow) { backend="reference"; reason="instance-capacity"; }
    else if (g_lagiBackendMode==LAGI_DSP_REFERENCE) { backend="reference"; reason="configured-reference"; }
    else if (g_lagiBackendMode!=LAGI_DSP_PREDECODED) {
        reason="aot-miss";
        if (g_lagiBackendMode!=LAGI_DSP_ARM) {
            for (i=0;i<g_lagiDspAotCount;++i) {
                const LagiDspAotEntry *entry=&g_lagiDspAotEntries[i];
                if (entry->hash==hash && entry->steps==steps && !memcmp(entry->words,DSP->MPRO,steps*8)) {
                    g_lagiNativeFunction=entry->function; backend="aot"; reason="aot-hit"; bytes=(unsigned)((uintptr_t)entry->codeEnd-(uintptr_t)entry->codeBegin); ++g_lagiCacheHits; break;
                }
            }
        }
        if (!g_lagiNativeFunction && g_lagiBackendMode!=LAGI_DSP_AOT) {
            if (!lagi_dsp_vm_available()) reason="vm-unavailable";
            else {
                for (i=0;i<4;++i) {
                    LagiDspCacheEntry *entry=&g_lagiNativeCache[i];
                    if (entry->function && entry->hash==hash && entry->steps==steps && !memcmp(entry->words,DSP->MPRO,steps*8)) {
                        g_lagiNativeFunction=entry->function; bytes=entry->bytes; ++g_lagiCacheHits; reason="arm-hit"; break;
                    }
                }
                if (!g_lagiNativeFunction && !allowCompile) {
                    g_lagiPendingDsp=DSP; reason="arm-pending";
                }
                if (!g_lagiNativeFunction && allowCompile) {
                    const unsigned slot=g_lagiCacheNext++%4;
                    LagiDspCacheEntry *entry=&g_lagiNativeCache[slot];
                    ++g_lagiCacheMisses; entry->function=0;
                    if (!lagi_dsp_decode_ir(&g_lagiTranslation,DSP->MPRO,steps)) reason="invalid-program";
                    else {
                        bytes=lagi_dsp_emit_arm(&g_lagiTranslation,g_lagiArmScratch,16384);
                        if (!bytes) reason="code-capacity";
                        else {
                            void *code=lagi_dsp_vm_publish(slot,g_lagiArmScratch,bytes);
                            if (!code) {
                                reason="publish-failed";
                                for (i=0;i<4;++i) g_lagiNativeCache[i].function=0;
                            } else {
                                entry->steps=steps; entry->hash=hash; entry->bytes=bytes;
                                memcpy(entry->words,DSP->MPRO,steps*8);
                                entry->function=(LagiDspNativeFn)code;
                                g_lagiNativeFunction=entry->function; reason="arm-compiled";
                            }
                        }
                    }
                }
                if (g_lagiNativeFunction) backend="arm";
            }
        } else if (!g_lagiNativeFunction) ++g_lagiCacheMisses;
    }
    g_lagiNativeBackend=!strcmp(backend,"arm")?3:(!strcmp(backend,"aot")?2:(!strcmp(backend,"predecoded")?1:0));
    lagi_dsp_selected_backend=g_lagiNativeBackend;
    lagi_dsp_selected_hash=hash;
    lagi_dsp_log("[LagiDSPProgram] hash=%08X steps=%u backend=%s translateUs=%llu codeBytes=%u sizeKind=%s hits=%u misses=%u reason=%s\n",
        hash,steps,backend,sceKernelGetSystemTimeWide()-started,bytes,
        !strcmp(backend,"aot")?"section":"runtime",g_lagiCacheHits,g_lagiCacheMisses,reason);
    if(!allowCompile) lagi_dsp_queue_capture(DSP,hash);
}

/* Called once per audio-worker iteration, outside the 256-frame sample loop. */
void lagi_dsp_service_pending(struct _SCSPDSP *DSP) {
    lagi_dsp_capture_one();
    if(g_lagiPendingDsp==DSP) {
        g_lagiPendingDsp=0;
        if(!DSP->Stopped && !lagi_dsp_is_dirty(DSP)) {
            if(g_lagiDecodedDsp!=DSP || g_lagiDecodedSteps!=DSP->LastStep) lagi_scspdsp_decode(DSP);
            lagi_dsp_select_native(DSP,1);
        }
    }
}
void lagi_dsp_release_native(void) {
    unsigned i;
    g_lagiNativeFunction=0; g_lagiNativeDsp=0; g_lagiPendingDsp=0;
    for(i=0;i<4;++i) g_lagiNativeCache[i].function=0;
    while(g_lagiCaptureRead!=g_lagiCaptureWrite) lagi_dsp_capture_one();
    lagi_dsp_selected_backend=0;
}
void SCSPDSP_Init(struct _SCSPDSP* DSP)
{
    if(g_lagiPendingDsp==DSP) g_lagiPendingDsp=0;
    lagi_dsp_clear_dirty(DSP,1);
    g_lagiBackendMode=lagi_dsp_platform_init();
    lagi_init_unpack_table();
    memset(DSP, 0, sizeof(struct _SCSPDSP));
    DSP->RBL = 0x8000;
    DSP->Stopped = 1;

    if (g_lagiNativeDsp == DSP) { g_lagiNativeFunction=0; g_lagiNativeDsp=0; }
    if (g_lagiDecodedDsp == DSP)
    {
        g_lagiDecodedDsp = 0;
        g_lagiDecodedSteps = 0;
        g_lagiPdsFastPath = 0;
    }
}

static inline INT32 lagi_sign_extend13_u32(UINT32 v)
{
    v &= 0x00001FFFu;
    if (v & 0x00001000u)
        v |= 0xFFFFE000u;
    return (INT32)v;
}

static inline INT32 lagi_sat24(INT64 v)
{
    if (v > 0x007FFFFF)
        return 0x007FFFFF;
    if (v < (-0x00800000))
        return -0x00800000;
    return v;
}

static inline INT32 lagi_sign_extend24_fast(UINT32 v)
{
    return ((INT32)(v << 8)) >> 8;
}

static void lagi_scspdsp_step_pds(struct _SCSPDSP* DSP)
{
    INT32 ACC = 0;
    INT32 SHIFTED = 0;
    INT32 INPUTS = 0;
    INT32 MEMVAL = 0;
    const UINT32 dec = DSP->DEC;
    const UINT32 rbp = DSP->RBP << 12;
    /* Index 0 is ring addressing, index 1 is table addressing. Select using
     * predecoded TABLE; values remain live for each sample, never at Start(). */
    const UINT32 addressOffset[2] = { dec, 0 };
    const UINT32 addressMask[2] = { DSP->RBL - 1, 0xFFFFu };
    int step;

    memset(DSP->EFREG, 0, 2 * 16);

    for (step = 0; step < DSP->LastStep; ++step)
    {
        const LagiPdsFastOp* op = &g_lagiPdsFastOps[step];
        const UINT8 f0 = op->flags0;
        const UINT8 f1 = op->flags1;
        INT32 B;
        INT32 X;
        INT32 tempValue = 0;

        INPUTS = lagi_sign_extend24_fast(
            (UINT32)*op->input << op->inputShift);

        if (f0 & LAGI_PDS_IWT)
        {
            DSP->MEMS[op->IWA] = MEMVAL;
            if (f1 & LAGI_PDS_IWT_ALIAS)
                INPUTS = MEMVAL;
        }

        /*
         * TEMP is needed by both B and X on many microinstructions. Read and
         * sign-extend it at most once per step when either source selects it.
         */
        if (f1 & LAGI_PDS_TEMP_USED)
            tempValue = lagi_sign_extend24_fast(
                (UINT32)DSP->TEMP[(op->TRA + dec) & 0x7F]);

        if (f1 & LAGI_PDS_ZERO)
            B = 0;
        else if (f1 & LAGI_PDS_BSEL)
            B = ACC;
        else
            B = tempValue;

        if (f1 & LAGI_PDS_NEGB)
            B = (INT32)(0u - (UINT32)B);

        X = (f0 & LAGI_PDS_XSEL) ? INPUTS : tempValue;

        /*
         * Pipeline ordering is intentional: SHIFTED comes from the accumulator
         * produced by the PREVIOUS microinstruction.
         */
        if (f1 & LAGI_PDS_SHIFT_USED)
            SHIFTED = lagi_sat24(
                (f1 & LAGI_PDS_SHIFT1) ? (INT64)ACC * 2 : (INT64)ACC);

        if (f1 & LAGI_PDS_YCOEF)
        {
            const INT32 Y = ((INT32)*op->coef >> 3);
            ACC = (INT32)((UINT32)(INT32)(((INT64)X * (INT64)Y) >> 12) + (UINT32)B);
        }
        else
        {
            /* FRC_REG is invariant zero for this guarded program class. */
            ACC = B;
        }

        if (f0 & LAGI_PDS_TWT)
            DSP->TEMP[(op->TWA + dec) & 0x7F] = SHIFTED;

        if (f0 & (LAGI_PDS_MRD | LAGI_PDS_MWT))
        {
            const unsigned addressMode = (f0 >> 6) & 1u;
            /* Resolve the register location at decode, not its live value.
             * Addition and masking retain UINT32 wrap before adding RBP. */
            UINT32 addr = *op->masa + addressOffset[addressMode];
            addr += op->addrIncrement;
            addr &= addressMask[addressMode];
            addr += rbp;

            if (f0 & LAGI_PDS_MRD)
                MEMVAL = UNPACK(DSP->SCSPRAM[addr]);
            if (f0 & LAGI_PDS_MWT)
                DSP->SCSPRAM[addr] = PACK(SHIFTED);
        }

        if (f0 & LAGI_PDS_EWT)
            DSP->EFREG[op->EWA] += SHIFTED >> 8;
    }

    --DSP->DEC;
    memset(DSP->MIXS, 0, 4 * 16);
}

void SCSPDSP_Step(struct _SCSPDSP* DSP)
{
    const int profileSample = lagi_dsp_profile_sample != 0;
    unsigned long long profileStartUs = 0;
    INT32 ACC = 0;
    INT32 SHIFTED = 0;
    INT32 X = 0;
    INT32 Y = 0;
    INT32 B = 0;
    INT32 INPUTS = 0;
    INT32 MEMVAL = 0;
    INT32 FRC_REG = 0;
    INT32 Y_REG = 0;
    UINT32 ADDR = 0;
    UINT32 ADRS_REG = 0;
    int step;

    if (profileSample)
    {
        lagi_dsp_profile_last_us = 0;
        profileStartUs = sceKernelGetSystemTimeWide();
    }

    if (DSP->Stopped)
    {
        if (profileSample)
            lagi_dsp_profile_last_us =
                sceKernelGetSystemTimeWide() - profileStartUs;
        return;
    }

    /*
     * Defensive fallback: normally Start() owns decoding. If an unusual path
     * changes LastStep or swaps the DSP instance, refresh before execution.
     */
    if (g_lagiDecodedDsp != DSP || g_lagiDecodedSteps != DSP->LastStep)
        lagi_scspdsp_decode(DSP);

    if (g_lagiNativeFunction && g_lagiNativeDsp==DSP && !lagi_dsp_is_dirty(DSP) && g_lagiBackendMode!=LAGI_DSP_REFERENCE) {
        lagi_dsp_selected_backend=g_lagiNativeBackend;
        g_lagiNativeFunction(DSP);
        if (profileSample) lagi_dsp_profile_last_us=sceKernelGetSystemTimeWide()-profileStartUs;
        return;
    }
    if (g_lagiPdsFastPath && !lagi_dsp_is_dirty(DSP) && g_lagiBackendMode!=LAGI_DSP_REFERENCE)
    {
        lagi_dsp_selected_backend=1;
        lagi_scspdsp_step_pds(DSP);
        if (profileSample)
            lagi_dsp_profile_last_us =
                sceKernelGetSystemTimeWide() - profileStartUs;
        return;
    }

    lagi_dsp_selected_backend=0;
    memset(DSP->EFREG, 0, 2 * 16);

    for (step = 0; step < DSP->LastStep; ++step)
    {
        const LagiScspDspOp* op = &g_lagiDspOps[step];
        INT64 v;

        assert(op->IRA < 0x32);
        if (op->IRA <= 0x1F)
            INPUTS = DSP->MEMS[op->IRA];
        else if (op->IRA <= 0x2F)
            INPUTS = (INT32)((UINT32)DSP->MIXS[op->IRA - 0x20] << 4);
        else
            INPUTS = 0;

        INPUTS = lagi_sign_extend24_u32((UINT32)INPUTS);

        if (op->IWT)
        {
            DSP->MEMS[op->IWA] = MEMVAL;
            if (op->IRA == op->IWA)
                INPUTS = MEMVAL;
        }

        if (!op->ZERO)
        {
            if (op->BSEL)
                B = ACC;
            else
            {
                B = lagi_sign_extend24_u32(
                    (UINT32)DSP->TEMP[(op->TRA + DSP->DEC) & 0x7F]);
            }
            if (op->NEGB)
                B = (INT32)(0u - (UINT32)B);
        }
        else
            B = 0;

        if (op->XSEL)
            X = INPUTS;
        else
        {
            X = lagi_sign_extend24_u32(
                (UINT32)DSP->TEMP[(op->TRA + DSP->DEC) & 0x7F]);
        }

        if (op->YSEL == 0)
            Y = FRC_REG;
        else if (op->YSEL == 1)
            Y = DSP->COEF[op->COEF] >> 3;
        else if (op->YSEL == 2)
            Y = (Y_REG >> 11) & 0x1FFF;
        else
            Y = (Y_REG >> 4) & 0x0FFF;

        if (op->YRL)
            Y_REG = INPUTS;

        if (op->SHIFT == 0)
        {
            SHIFTED = lagi_sat24((INT64)ACC);
        }
        else if (op->SHIFT == 1)
        {
            SHIFTED = lagi_sat24((INT64)ACC * 2);
        }
        else if (op->SHIFT == 2)
        {
            SHIFTED = lagi_sign_extend24_u32((UINT32)ACC << 1);
        }
        else
        {
            SHIFTED = lagi_sign_extend24_u32((UINT32)ACC);
        }

        Y = lagi_sign_extend13_u32((UINT32)Y);

        v = (((INT64)X * (INT64)Y) >> 12);
        ACC = (INT32)((UINT32)(INT32)v + (UINT32)B);

        if (op->TWT)
            DSP->TEMP[(op->TWA + DSP->DEC) & 0x7F] = SHIFTED;

        if (op->FRCL)
        {
            if (op->SHIFT == 3)
                FRC_REG = SHIFTED & 0x0FFF;
            else
                FRC_REG = (SHIFTED >> 11) & 0x1FFF;
        }

        if (op->MRD || op->MWT)
        {
            ADDR = DSP->MADRS[op->MASA];
            if (!op->TABLE)
                ADDR += DSP->DEC;
            if (op->ADREB)
                ADDR += ADRS_REG & 0x0FFF;
            if (op->NXADR)
                ADDR++;
            if (!op->TABLE)
                ADDR &= DSP->RBL - 1;
            else
                ADDR &= 0xFFFF;

            ADDR += DSP->RBP << 12;

            if (op->MRD && op->odd)
            {
                if (op->NOFL)
                    MEMVAL = DSP->SCSPRAM[ADDR] << 8;
                else
                    MEMVAL = UNPACK(DSP->SCSPRAM[ADDR]);
            }

            if (op->MWT && op->odd)
            {
                if (op->NOFL)
                    DSP->SCSPRAM[ADDR] = SHIFTED >> 8;
                else
                    DSP->SCSPRAM[ADDR] = PACK(SHIFTED);
            }
        }

        if (op->ADRL)
        {
            if (op->SHIFT == 3)
                ADRS_REG = (SHIFTED >> 12) & 0xFFF;
            else
                ADRS_REG = INPUTS >> 16;
        }

        if (op->EWT)
            DSP->EFREG[op->EWA] += SHIFTED >> 8;
    }

    --DSP->DEC;
    memset(DSP->MIXS, 0, 4 * 16);

    if (profileSample)
        lagi_dsp_profile_last_us =
            sceKernelGetSystemTimeWide() - profileStartUs;
}

void SCSPDSP_SetSample(struct _SCSPDSP* DSP, INT32 sample, int SEL, int MXL)
{
    (void)MXL;
    DSP->MIXS[SEL] += sample;
}

void SCSPDSP_Start(struct _SCSPDSP* DSP)
{
    int i;

    DSP->Stopped = 0;
    for (i = 127; i >= 0; --i)
    {
        UINT16* IPtr = DSP->MPRO + i * 4;
        if (IPtr[0] != 0 || IPtr[1] != 0 || IPtr[2] != 0 || IPtr[3] != 0)
            break;
    }

    DSP->LastStep = i + 1;
    lagi_scspdsp_decode(DSP);
    lagi_dsp_select_native(DSP,0);

    {
        unsigned nIWT = 0, nTWT = 0, nMRD = 0, nMWT = 0, nEWT = 0;
        unsigned nADRL = 0, nFRCL = 0, nYRL = 0, nXInput = 0;
        unsigned nYCoef = 0, nYFrc = 0, nYReg = 0;
        unsigned nShiftSat = 0, nShiftWrap = 0;
        unsigned nPack = 0, nUnpack = 0, nNoflRead = 0, nNoflWrite = 0;
        int s;

        for (s = 0; s < DSP->LastStep; ++s)
        {
            const LagiScspDspOp* op = &g_lagiDspOps[s];
            nIWT += op->IWT != 0;
            nTWT += op->TWT != 0;
            nMRD += op->MRD != 0 && op->odd;
            nMWT += op->MWT != 0 && op->odd;
            nEWT += op->EWT != 0;
            nADRL += op->ADRL != 0;
            nFRCL += op->FRCL != 0;
            nYRL += op->YRL != 0;
            nXInput += op->XSEL != 0;

            if (op->YSEL == 0) ++nYFrc;
            else if (op->YSEL == 1) ++nYCoef;
            else ++nYReg;

            if (op->SHIFT <= 1) ++nShiftSat;
            else ++nShiftWrap;

            if (op->MRD && op->odd)
            {
                if (op->NOFL) ++nNoflRead;
                else ++nUnpack;
            }
            if (op->MWT && op->odd)
            {
                if (op->NOFL) ++nNoflWrite;
                else ++nPack;
            }
        }

        lagi_dsp_mix_steps = (unsigned)DSP->LastStep;
        lagi_dsp_mix_iwt = nIWT;
        lagi_dsp_mix_twt = nTWT;
        lagi_dsp_mix_mrd = nMRD;
        lagi_dsp_mix_mwt = nMWT;
        lagi_dsp_mix_ewt = nEWT;
        lagi_dsp_mix_adrl = nADRL;
        lagi_dsp_mix_frcl = nFRCL;
        lagi_dsp_mix_yrl = nYRL;
        lagi_dsp_mix_xinput = nXInput;
        lagi_dsp_mix_yfrc = nYFrc;
        lagi_dsp_mix_ycoef = nYCoef;
        lagi_dsp_mix_yreg = nYReg;
        lagi_dsp_mix_satshift = nShiftSat;
        lagi_dsp_mix_wrapshift = nShiftWrap;
        lagi_dsp_mix_unpack = nUnpack;
        lagi_dsp_mix_pack = nPack;
        lagi_dsp_mix_noflr = nNoflRead;
        lagi_dsp_mix_noflw = nNoflWrite;
        ++lagi_dsp_mix_serial;
    }
}

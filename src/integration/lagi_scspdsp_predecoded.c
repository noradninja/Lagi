#include <assert.h>
#include <string.h>

#include "ao.h"
#include "cpuintrf.h"
#include "scsp.h"

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

typedef struct LagiScspDspOp {
    UINT8 TRA;
    UINT8 TWT;
    UINT8 TWA;

    UINT8 XSEL;
    UINT8 YSEL;
    UINT8 IRA;
    UINT8 IWT;
    UINT8 IWA;

    UINT8 TABLE;
    UINT8 MWT;
    UINT8 MRD;
    UINT8 EWT;
    UINT8 EWA;
    UINT8 ADRL;
    UINT8 FRCL;
    UINT8 SHIFT;
    UINT8 YRL;
    UINT8 NEGB;
    UINT8 ZERO;
    UINT8 BSEL;

    UINT8 NOFL;
    UINT8 COEF;
    UINT8 MASA;
    UINT8 ADREB;
    UINT8 NXADR;
    UINT8 odd;
} LagiScspDspOp;

static LagiScspDspOp g_lagiDspOps[128];
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

static UINT16 PACK(INT32 val)
{
    UINT32 temp;
    int sign, exponent;

    sign = (val >> 23) & 0x1;
    temp = (val ^ (val << 1)) & 0xFFFFFFu;

    /*
     * Original AOSDK scans bit 23 leftward for at most 12 iterations.
     * CLZ gives the identical exponent in one ARM instruction.
     */
    if (temp == 0)
        exponent = 12;
    else
    {
        exponent = __builtin_clz(temp) - 8;
        if (exponent > 12)
            exponent = 12;
    }

    if (exponent < 12)
        val = (val << exponent) & 0x3FFFFF;
    else
        val <<= 11;
    val >>= 11;
    val &= 0x7FF;
    val |= sign << 15;
    val |= exponent << 11;

    return (UINT16)val;
}

static INT32 UNPACK(UINT16 val)
{
    int sign, exponent, mantissa;
    INT32 uval;

    sign = (val >> 15) & 0x1;
    exponent = (val >> 11) & 0xF;
    mantissa = val & 0x7FF;
    uval = mantissa << 11;
    if (exponent > 11)
    {
        exponent = 11;
        uval |= sign << 22;
    }
    else
        uval |= (sign ^ 1) << 22;
    uval |= sign << 23;
    uval <<= 8;
    uval >>= 8;
    uval >>= exponent;

    return uval;
}

static void lagi_scspdsp_decode(struct _SCSPDSP* DSP)
{
    int step;

    g_lagiDecodedDsp = DSP;
    g_lagiDecodedSteps = DSP->LastStep;

    for (step = 0; step < DSP->LastStep; ++step)
    {
        UINT16* IPtr = DSP->MPRO + step * 4;
        LagiScspDspOp* op = &g_lagiDspOps[step];

        op->TRA   = (UINT8)((IPtr[0] >> 8) & 0x7F);
        op->TWT   = (UINT8)((IPtr[0] >> 7) & 0x01);
        op->TWA   = (UINT8)((IPtr[0] >> 0) & 0x7F);

        op->XSEL  = (UINT8)((IPtr[1] >> 15) & 0x01);
        op->YSEL  = (UINT8)((IPtr[1] >> 13) & 0x03);
        op->IRA   = (UINT8)((IPtr[1] >> 6) & 0x3F);
        op->IWT   = (UINT8)((IPtr[1] >> 5) & 0x01);
        op->IWA   = (UINT8)((IPtr[1] >> 0) & 0x1F);

        op->TABLE = (UINT8)((IPtr[2] >> 15) & 0x01);
        op->MWT   = (UINT8)((IPtr[2] >> 14) & 0x01);
        op->MRD   = (UINT8)((IPtr[2] >> 13) & 0x01);
        op->EWT   = (UINT8)((IPtr[2] >> 12) & 0x01);
        op->EWA   = (UINT8)((IPtr[2] >> 8) & 0x0F);
        op->ADRL  = (UINT8)((IPtr[2] >> 7) & 0x01);
        op->FRCL  = (UINT8)((IPtr[2] >> 6) & 0x01);
        op->SHIFT = (UINT8)((IPtr[2] >> 4) & 0x03);
        op->YRL   = (UINT8)((IPtr[2] >> 3) & 0x01);
        op->NEGB  = (UINT8)((IPtr[2] >> 2) & 0x01);
        op->ZERO  = (UINT8)((IPtr[2] >> 1) & 0x01);
        op->BSEL  = (UINT8)((IPtr[2] >> 0) & 0x01);

        op->NOFL  = (UINT8)((IPtr[3] >> 15) & 0x01);
        op->COEF  = (UINT8)((IPtr[3] >> 9) & 0x3F);
        op->MASA  = (UINT8)((IPtr[3] >> 2) & 0x1F);
        op->ADREB = (UINT8)((IPtr[3] >> 1) & 0x01);
        op->NXADR = (UINT8)((IPtr[3] >> 0) & 0x01);
        op->odd   = (UINT8)(step & 1);
    }

    /*
     * PDS DSP banks observed so far share a much narrower instruction subset:
     * - Y source is only FRC or COEF
     * - FRCL/YRL/ADRL are never used, so FRC/Y/ADRS registers stay zero
     * - SHIFT is only saturating mode 0 or 1
     * - SCSP RAM traffic always uses PACK/UNPACK (NOFL=0)
     *
     * Use a specialized loop only when the entire loaded program satisfies
     * those invariants. Unknown banks retain the full interpreter below.
     */
    /*
     * First optimization pass: generic predecode only.
     * Do not enable the specialized PDS instruction-subset fast path yet;
     * this keeps arithmetic and memory behavior on the full interpreter path
     * while removing repeated MPRO bitfield extraction.
     */
    g_lagiPdsFastPath = 0;
    lagi_dsp_fast_path = 0;
}

void SCSPDSP_Init(struct _SCSPDSP* DSP)
{
    memset(DSP, 0, sizeof(struct _SCSPDSP));
    DSP->RBL = 0x8000;
    DSP->Stopped = 1;

    if (g_lagiDecodedDsp == DSP)
    {
        g_lagiDecodedDsp = 0;
        g_lagiDecodedSteps = 0;
        g_lagiPdsFastPath = 0;
    }
}

static inline INT32 lagi_sign_extend24(INT32 v)
{
    v <<= 8;
    v >>= 8;
    return v;
}

static inline INT32 lagi_sat24(INT32 v)
{
    if (v > 0x007FFFFF)
        return 0x007FFFFF;
    if (v < (-0x00800000))
        return -0x00800000;
    return v;
}

static void lagi_scspdsp_step_pds(struct _SCSPDSP* DSP)
{
    INT32 ACC = 0;
    INT32 SHIFTED = 0;
    INT32 INPUTS = 0;
    INT32 MEMVAL = 0;
    int step;

    memset(DSP->EFREG, 0, 2 * 16);

    for (step = 0; step < DSP->LastStep; ++step)
    {
        const LagiScspDspOp* op = &g_lagiDspOps[step];
        INT32 B;
        INT32 X;

        if (op->IRA <= 0x1F)
            INPUTS = DSP->MEMS[op->IRA];
        else if (op->IRA <= 0x2F)
            INPUTS = DSP->MIXS[op->IRA - 0x20] << 4;
        else
            INPUTS = 0;
        INPUTS = lagi_sign_extend24(INPUTS);

        if (op->IWT)
        {
            DSP->MEMS[op->IWA] = MEMVAL;
            if (op->IRA == op->IWA)
                INPUTS = MEMVAL;
        }

        if (op->ZERO)
            B = 0;
        else if (op->BSEL)
            B = ACC;
        else
            B = lagi_sign_extend24(
                DSP->TEMP[(op->TRA + DSP->DEC) & 0x7F]);

        if (op->NEGB)
            B = -B;

        if (op->XSEL)
            X = INPUTS;
        else
            X = lagi_sign_extend24(
                DSP->TEMP[(op->TRA + DSP->DEC) & 0x7F]);

        /*
         * SCSP DSP pipeline ordering is significant: SHIFTED is derived from
         * the accumulator value produced by the PREVIOUS microinstruction.
         * Only after SHIFTED is latched does this instruction calculate the
         * next ACC value. Do not reorder these operations.
         */
        SHIFTED = lagi_sat24(op->SHIFT ? ACC * 2 : ACC);

        /*
         * FRCL is absent in this program class, so FRC_REG remains zero.
         * YSEL=0 therefore contributes a guaranteed zero product.
         */
        if (op->YSEL == 0)
        {
            ACC = B;
        }
        else
        {
            INT32 Y = DSP->COEF[op->COEF] >> 3;
            Y = (Y << 19) >> 19;
            ACC = (INT32)(((INT64)X * (INT64)Y) >> 12) + B;
        }

        if (op->TWT)
            DSP->TEMP[(op->TWA + DSP->DEC) & 0x7F] = SHIFTED;

        if ((op->MRD || op->MWT) && op->odd)
        {
            UINT32 addr = DSP->MADRS[op->MASA];

            if (!op->TABLE)
                addr += DSP->DEC;

            /*
             * ADRL is absent, so ADRS_REG remains zero. ADREB therefore adds
             * nothing even when encoded.
             */
            if (op->NXADR)
                ++addr;

            if (!op->TABLE)
                addr &= DSP->RBL - 1;
            else
                addr &= 0xFFFF;

            addr += DSP->RBP << 12;

            if (op->MRD)
                MEMVAL = UNPACK(DSP->SCSPRAM[addr]);

            if (op->MWT)
                DSP->SCSPRAM[addr] = PACK(SHIFTED);
        }

        if (op->EWT)
            DSP->EFREG[op->EWA] += SHIFTED >> 8;
    }

    --DSP->DEC;
    memset(DSP->MIXS, 0, 4 * 16);
}

void SCSPDSP_Step(struct _SCSPDSP* DSP)
{
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

    if (DSP->Stopped)
        return;

    /*
     * Defensive fallback: normally Start() owns decoding. If an unusual path
     * changes LastStep or swaps the DSP instance, refresh before execution.
     */
    if (g_lagiDecodedDsp != DSP || g_lagiDecodedSteps != DSP->LastStep)
        lagi_scspdsp_decode(DSP);

    if (g_lagiPdsFastPath)
    {
        lagi_scspdsp_step_pds(DSP);
        return;
    }

    memset(DSP->EFREG, 0, 2 * 16);

    for (step = 0; step < DSP->LastStep; ++step)
    {
        const LagiScspDspOp* op = &g_lagiDspOps[step];
        INT64 v;

        assert(op->IRA < 0x32);
        if (op->IRA <= 0x1F)
            INPUTS = DSP->MEMS[op->IRA];
        else if (op->IRA <= 0x2F)
            INPUTS = DSP->MIXS[op->IRA - 0x20] << 4;
        else
            INPUTS = 0;

        INPUTS <<= 8;
        INPUTS >>= 8;

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
                B = DSP->TEMP[(op->TRA + DSP->DEC) & 0x7F];
                B <<= 8;
                B >>= 8;
            }
            if (op->NEGB)
                B = 0 - B;
        }
        else
            B = 0;

        if (op->XSEL)
            X = INPUTS;
        else
        {
            X = DSP->TEMP[(op->TRA + DSP->DEC) & 0x7F];
            X <<= 8;
            X >>= 8;
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
            SHIFTED = ACC;
            if (SHIFTED > 0x007FFFFF)
                SHIFTED = 0x007FFFFF;
            if (SHIFTED < (-0x00800000))
                SHIFTED = -0x00800000;
        }
        else if (op->SHIFT == 1)
        {
            SHIFTED = ACC * 2;
            if (SHIFTED > 0x007FFFFF)
                SHIFTED = 0x007FFFFF;
            if (SHIFTED < (-0x00800000))
                SHIFTED = -0x00800000;
        }
        else if (op->SHIFT == 2)
        {
            SHIFTED = ACC * 2;
            SHIFTED <<= 8;
            SHIFTED >>= 8;
        }
        else
        {
            SHIFTED = ACC;
            SHIFTED <<= 8;
            SHIFTED >>= 8;
        }

        Y <<= 19;
        Y >>= 19;

        v = (((INT64)X * (INT64)Y) >> 12);
        ACC = (int)v + B;

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

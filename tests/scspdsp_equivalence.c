#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lagi_scspdsp_predecoded.c"

static uint32_t seed = 0x597fb08u;
static uint32_t random32(void) { seed ^= seed << 13; seed ^= seed >> 17; seed ^= seed << 5; return seed; }
static struct _SCSPDSP a, b;
static UINT16 ramA[131072], ramB[131072];

static void equal_state(unsigned program, unsigned sample)
{
    struct _SCSPDSP ca = a, cb = b;
    ca.SCSPRAM = cb.SCSPRAM = NULL;
    if (memcmp(&ca, &cb, sizeof(ca)) || memcmp(ramA, ramB, sizeof(ramA))) {
        fprintf(stderr, "DSP mismatch program=%u sample=%u\n", program, sample);
        exit(1);
    }
}

int main(void)
{
    unsigned program, sample, i;
    for (program = 0; program < 32; ++program) {
        SCSPDSP_Init(&a);
        a.SCSPRAM = ramA;
        a.SCSPRAM_LENGTH = sizeof(ramA);
        for (i = 0; i < 131072; ++i) ramA[i] = (UINT16)random32();
        for (i = 0; i < 128; ++i) a.TEMP[i] = lagi_sign_extend24_u32(random32());
        for (i = 0; i < 32; ++i) a.MEMS[i] = lagi_sign_extend24_u32(random32());
        for (i = 0; i < 84; ++i) {
            /* Every guarded control combination, odd/even RAM operations,
             * aliasing, saturation, table/ring and NXADR are randomized. */
            UINT16 *op = &a.MPRO[i * 4];
            op[0] = (UINT16)random32() & 0x7fff;
            op[1] = ((random32() & 1) << 15) | ((random32() & 1) << 13) |
                    ((random32() % 50) << 6) | (random32() & 63);
            if (i % 7 == 0) op[1] = (op[1] & ~0xfc0) | ((op[1] & 31) << 6);
            op[2] = (UINT16)random32() & 0xff17;
            op[3] = (UINT16)random32() & 0x7fff;
        }
        a.MPRO[83 * 4] |= 0x80;
        SCSPDSP_Start(&a);
        if (!g_lagiPdsFastPath || a.LastStep != 84) return 2;
        b = a; b.SCSPRAM = ramB;
        memcpy(ramB, ramA, sizeof(ramA));
        for (sample = 0; sample < 32; ++sample) {
            /* Mutate live inputs between samples without Start(). */
            for (i = 0; i < 64; ++i) a.COEF[i] = b.COEF[i] = (INT16)random32();
            for (i = 0; i < 32; ++i) a.MADRS[i] = b.MADRS[i] = (UINT16)random32();
            for (i = 0; i < 16; ++i) a.MIXS[i] = b.MIXS[i] = (INT32)(random32() & 0xfffff);
            a.DEC = b.DEC = sample % 3 == 0 ? UINT32_MAX - sample : random32();
            a.RBL = b.RBL = 0x2000u << (sample % 3);
            a.RBP = b.RBP = sample % 4;
            lagi_scspdsp_decode(&a);
            SCSPDSP_Step(&a);
            lagi_scspdsp_decode(&b);
            g_lagiPdsFastPath = 0;
            SCSPDSP_Step(&b);
            equal_state(program, sample);
        }
    }
    /* Each excluded instruction feature must select the generic fallback. */
    for (i = 0; i < 7; ++i) {
        SCSPDSP_Init(&a);
        if (i == 0) a.MPRO[1] = 2u << 13;
        if (i == 1) a.MPRO[1] = 3u << 13;
        if (i == 2) a.MPRO[2] = 1u << 6;
        if (i == 3) a.MPRO[2] = 1u << 3;
        if (i == 4) a.MPRO[2] = 1u << 7;
        if (i == 5) a.MPRO[2] = 2u << 4;
        if (i == 6) a.MPRO[3] = 1u << 15;
        SCSPDSP_Start(&a);
        if (g_lagiPdsFastPath) return 3;
    }
    puts("PASS: 1,024 84-step fast/generic comparisons; live COEF/MADRS, wrap, RAM, registers, fallback guards");
    return 0;
}

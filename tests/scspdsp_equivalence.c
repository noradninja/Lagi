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


static struct _SCSPDSP nativeC, nativeArm;
static UINT16 ramC[131072], ramArm[131072];
static UINT32 armCode[16384];
static void compare_native(struct _SCSPDSP *state, UINT16 *ram) {
    struct _SCSPDSP expected=a, actual=*state;
    expected.SCSPRAM=actual.SCSPRAM=0;
    if (memcmp(&expected,&actual,sizeof(actual)) || memcmp(ramA,ram,sizeof(ramA))) exit(10);
}
static INT32 reference_unpack(UINT16 val) {
    unsigned sign=val>>15, exponent=(val>>11)&15, mantissa=val&2047;
    unsigned raw=mantissa*2048;
    if(exponent>11) { exponent=11; raw+=sign*4194304; }
    else raw+=(sign^1)*4194304;
    raw+=sign*8388608;
    INT32 signedValue=(INT32)raw-(sign?16777216:0);
    const INT32 divisor=1<<exponent;
    return signedValue<0?(signedValue-(divisor-1))/divisor:signedValue/divisor;
}
static UINT16 reference_pack(INT32 value) {
    UINT32 raw=(UINT32)value, sign=(raw>>23)&1, temp=(raw^(raw<<1))&0xffffff;
    unsigned exponent=0;
    while(exponent<12 && !(temp&0x800000)) { temp<<=1; ++exponent; }
    unsigned mantissa=exponent<12?((raw<<exponent)&0x3fffff)>>11:raw&2047;
    return (UINT16)(mantissa|(sign<<15)|(exponent<<11));
}
static __attribute__((naked)) int native_abi(LagiDspNativeFn fn,struct _SCSPDSP *DSP) {
    __asm__ volatile(
        "push {r4-r12,lr}\nmov r12,r0\nmov r0,r1\n"
        "mov r4,#4\nmov r5,#5\nmov r6,#6\nmov r7,#7\n"
        "mov r8,#8\nmov r9,#9\nmov r10,#10\nmov r11,#11\nblx r12\n"
        "cmp r4,#4\nbne 1f\ncmp r5,#5\nbne 1f\ncmp r6,#6\nbne 1f\ncmp r7,#7\nbne 1f\n"
        "cmp r8,#8\nbne 1f\ncmp r9,#9\nbne 1f\ncmp r10,#10\nbne 1f\ncmp r11,#11\nbne 1f\n"
        "mov r0,#0\nb 2f\n1: mov r0,#1\n2: pop {r4-r12,pc}\n");
}
static void native_tests(void) {
    unsigned p,s,i;
    {
        const INT32 limits[]={INT32_MIN,INT32_MAX,-8388609,-8388608,-4194305,-4194304,-1,0,1,4194303,4194304,8388607,8388608};
        for(i=0;i<sizeof(limits)/sizeof(limits[0]);++i)
            if(lagi_dsp_native_sat(limits[i],0)!=lagi_sat24((INT64)limits[i]) ||
               lagi_dsp_native_sat(limits[i],1)!=lagi_sat24((INT64)limits[i]*2)) exit(19);
    }
    for(i=0;i<65536;++i) {
        INT32 value=reference_unpack((UINT16)i);
        if(UNPACK((UINT16)i)!=value || PACK(value)!=reference_pack(value)) exit(15);
        value=(INT32)(i*256u)-8388608;
        if(PACK(value)!=reference_pack(value)) exit(16);
    }
    for(p=0;p<g_lagiDspAotCount;++p) {
        const LagiDspAotEntry *entry=&g_lagiDspAotEntries[p];
        SCSPDSP_Init(&a); a.SCSPRAM=ramA; a.SCSPRAM_LENGTH=sizeof(ramA);
        memcpy(a.MPRO,entry->words,sizeof(a.MPRO));
        SCSPDSP_Start(&a); lagi_dsp_service_pending(&a);
        if(a.LastStep!=(int)entry->steps) exit(11);
        g_lagiBackendMode=LAGI_DSP_REFERENCE;
        for(i=0;i<131072;++i) ramA[i]=(UINT16)random32();
        for(i=0;i<128;++i) a.TEMP[i]=(INT32)random32();
        for(i=0;i<32;++i) a.MEMS[i]=(INT32)random32();
        nativeC=nativeArm=a; nativeC.SCSPRAM=ramC; nativeArm.SCSPRAM=ramArm;
        memcpy(ramC,ramA,sizeof(ramA)); memcpy(ramArm,ramA,sizeof(ramA));
        if(!lagi_dsp_decode_ir(&g_lagiTranslation,a.MPRO,a.LastStep)) exit(12);
        if(!lagi_dsp_emit_arm(&g_lagiTranslation,armCode,16384)) exit(13);
        for(s=0;s<16;++s) {
            for(i=0;i<64;++i) a.COEF[i]=nativeC.COEF[i]=nativeArm.COEF[i]=(INT16)random32();
            for(i=0;i<32;++i) a.MADRS[i]=nativeC.MADRS[i]=nativeArm.MADRS[i]=(UINT16)random32();
            for(i=0;i<16;++i) a.MIXS[i]=nativeC.MIXS[i]=nativeArm.MIXS[i]=(INT32)random32();
            a.DEC=nativeC.DEC=nativeArm.DEC=s%3==0?UINT32_MAX-s:random32();
            a.RBL=nativeC.RBL=nativeArm.RBL=s%4==0?0x3456u:(0x2000u << (s%4));
            a.RBP=nativeC.RBP=nativeArm.RBP=s%4;
            SCSPDSP_Step(&a);
            if(native_abi(entry->function,&nativeC)) exit(17);
            if(native_abi((LagiDspNativeFn)armCode,&nativeArm)) exit(18);
            compare_native(&nativeC,ramC); compare_native(&nativeArm,ramArm);
        }
        /* IR validity and bounded writer failure never publish partial code. */
        armCode[0]=0xdeadbeef; armCode[1]=0xfeedface;
        if(lagi_dsp_emit_arm(&g_lagiTranslation,armCode,1)!=0 || armCode[1]!=0xfeedface) exit(14);
    }
    /* Selection uses full words, including when a hash/length match is forced. */
    SCSPDSP_Init(&a); a.SCSPRAM=ramA;
    memcpy(a.MPRO,g_lagiDspAotEntries[0].words,sizeof(a.MPRO)); SCSPDSP_Start(&a); lagi_dsp_service_pending(&a);
    lagi_dsp_profile_sample=1; SCSPDSP_Step(&a); lagi_dsp_profile_sample=0;
    if(lagi_dsp_profile_backend_last!=1) exit(45);
    g_lagiBackendMode=LAGI_DSP_AOT; lagi_dsp_select_native(&a,1);
    if(g_lagiNativeFunction!=g_lagiDspAotEntries[0].function) exit(20);
    lagi_dsp_profile_sample=1; SCSPDSP_Step(&a); lagi_dsp_profile_sample=0;
    if(lagi_dsp_profile_backend_last!=2) exit(43);
    a.Stopped=1; b=a; SCSPDSP_Step(&a); if(memcmp(&a,&b,sizeof(a))) exit(21); a.Stopped=0;
    lagi_dsp_program_written(&a); a.MPRO[0]^=0x80;
    if(g_lagiNativeFunction || !lagi_dsp_is_dirty(&a)) exit(22);
    /* Dirty execution refreshes decode but never reinstalls native code. */
    lagi_dsp_profile_sample=1; SCSPDSP_Step(&a); lagi_dsp_profile_sample=0;
    if(g_lagiNativeFunction || lagi_dsp_profile_backend_last!=0) exit(46);
    SCSPDSP_Start(&a); lagi_dsp_service_pending(&a); if(lagi_dsp_is_dirty(&a) || g_lagiNativeFunction) exit(24);
    g_lagiBackendMode=LAGI_DSP_ARM; g_lagiDspTestVm=0; lagi_dsp_select_native(&a,1);
    if(g_lagiNativeFunction) exit(25);
    g_lagiDspTestVm=1; lagi_dsp_select_native(&a,1);
    if(!g_lagiNativeFunction) exit(26);
    lagi_dsp_profile_sample=1; SCSPDSP_Step(&a); lagi_dsp_profile_sample=0;
    if(lagi_dsp_profile_backend_last!=3) exit(44);
    i=g_lagiCacheHits; lagi_dsp_select_native(&a,1);
    if(g_lagiCacheHits!=i+1) exit(27);
    /* Collision: mutate content while keeping the cache key equal. */
    for(i=0;i<4;++i) if(g_lagiNativeCache[i].function) g_lagiNativeCache[i].words[0]^=1;
    i=g_lagiCacheMisses; lagi_dsp_select_native(&a,1);
    if(g_lagiCacheMisses!=i+1) exit(28);
    b=a; b.SCSPRAM=ramB; memcpy(ramB,ramA,sizeof(ramA));
    SCSPDSP_Start(&b); lagi_dsp_service_pending(&b); /* native function takes current instance, no fixed DSP pointers */
    if(g_lagiNativeDsp!=&b || !g_lagiNativeFunction) exit(29);
    lagi_dsp_program_written(&a);
    if(!g_lagiNativeFunction || !lagi_dsp_is_dirty(&a) || lagi_dsp_is_dirty(&b)) exit(34);
    SCSPDSP_Step(&a); if(lagi_dsp_selected_backend!=0) exit(35);
    SCSPDSP_Step(&b); if(lagi_dsp_selected_backend!=3) exit(36);
    for(i=0;i<6;++i) { a.MPRO[0]=(g_lagiDspAotEntries[0].words[0]+i+1)&0x7fff; SCSPDSP_Start(&a); lagi_dsp_service_pending(&a); if(!g_lagiNativeFunction) exit(37); }
    for(i=0;i<4;++i) g_lagiNativeCache[i].function=0;
    g_lagiDspTestPublishFailure=1;
    a.MPRO[0]^=0x100; SCSPDSP_Start(&a); lagi_dsp_service_pending(&a);
    if(g_lagiNativeFunction) exit(30);
    g_lagiDspTestPublishFailure=0;
    SCSPDSP_Init(&a); a.SCSPRAM=ramA; SCSPDSP_Start(&a); lagi_dsp_service_pending(&a);
    g_lagiBackendMode=LAGI_DSP_ARM; lagi_dsp_select_native(&a,1);
    if(!g_lagiNativeFunction) exit(31);
    a.DEC=0; SCSPDSP_Step(&a); if(a.DEC!=UINT32_MAX) exit(32);
    /* A cache miss at Start must defer all emission/publication to the boundary. */
    SCSPDSP_Init(&a); a.SCSPRAM=ramA;
    memcpy(a.MPRO,g_lagiDspAotEntries[0].words,sizeof(a.MPRO));
    g_lagiBackendMode=LAGI_DSP_ARM;
    for(i=0;i<4;++i) g_lagiNativeCache[i].function=0;
    i=g_lagiCacheMisses; SCSPDSP_Start(&a);
    if(g_lagiNativeFunction || g_lagiPendingDsp!=&a || g_lagiCacheMisses!=i) exit(39);
    lagi_dsp_service_pending(&a); if(!g_lagiNativeFunction) exit(40);
    a.MPRO[0]^=1; SCSPDSP_Start(&a);
    if(g_lagiNativeFunction || g_lagiPendingDsp!=&a) exit(41);
    lagi_dsp_program_written(&a); a.MPRO[1]^=0x20;
    i=g_lagiCacheMisses; lagi_dsp_service_pending(&a);
    if(g_lagiNativeFunction || g_lagiPendingDsp || g_lagiCacheMisses!=i) exit(42);
    SCSPDSP_Init(&a); a.SCSPRAM=ramA; a.MPRO[1]=63u << 6;
    SCSPDSP_Start(&a); lagi_dsp_service_pending(&a); g_lagiBackendMode=LAGI_DSP_ARM; lagi_dsp_select_native(&a,1);
    if(g_lagiNativeFunction) exit(33);
    lagi_dsp_release_native(); if(g_lagiNativeFunction || g_lagiPendingDsp) exit(38);
    g_lagiBackendMode=LAGI_DSP_PREDECODED;
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
        SCSPDSP_Start(&a); lagi_dsp_service_pending(&a);
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
        SCSPDSP_Start(&a); lagi_dsp_service_pending(&a);
        if (g_lagiPdsFastPath) return 3;
    }
    native_tests();
    puts("PASS: 1,024 84-step fast/generic comparisons; live COEF/MADRS, wrap, RAM, registers, fallback guards");
    return 0;
}

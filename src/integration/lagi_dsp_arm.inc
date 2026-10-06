/* ARMv7 A32 emitter. Generated functions use AAPCS and contain no DSP opcode
 * dispatch. Only saturation and packed-format conversions call shared helpers. */
#include <stddef.h>
#include <arm_acle.h>
_Static_assert(sizeof(void*)==4,"DSP ARM backend requires 32-bit pointers");
_Static_assert(offsetof(struct _SCSPDSP,LastStep)<4096,"DSP ARM load/store layout exceeded");
typedef struct LagiDspArmWriter { UINT32 *code; unsigned used, capacity; int failed; } LagiDspArmWriter;
static void lagi_arm_word(LagiDspArmWriter *w, UINT32 word) {
    if (w->used < w->capacity) w->code[w->used++]=word;
    else w->failed=1;
}
static void lagi_arm_imm(LagiDspArmWriter *w, unsigned rd, UINT32 value) {
    if (value < 256) lagi_arm_word(w,0xe3a00000u | rd<<12 | value);
    else {
        lagi_arm_word(w,0xe3000000u | (value&0xf000u)<<4 | rd<<12 | (value&0xfffu));
        if (value>>16) { value>>=16; lagi_arm_word(w,0xe3400000u | (value&0xf000u)<<4 | rd<<12 | (value&0xfffu)); }
    }
}
static void lagi_arm_dp(LagiDspArmWriter *w,unsigned opcode,unsigned rd,unsigned rn,unsigned rm,unsigned shift,unsigned amount) {
    lagi_arm_word(w,0xe0000000u | opcode<<21 | rn<<16 | rd<<12 | amount<<7 | shift<<5 | rm);
}
static void lagi_arm_mov(LagiDspArmWriter *w,unsigned rd,unsigned rm,unsigned shift,unsigned amount) {
    lagi_arm_dp(w,13,rd,0,rm,shift,amount);
}
static void lagi_arm_small(LagiDspArmWriter *w,unsigned opcode,unsigned rd,unsigned rn,unsigned value) {
    lagi_arm_word(w,0xe2000000u | opcode<<21 | rn<<16 | rd<<12 | value);
}
static void lagi_arm_load(LagiDspArmWriter *w,unsigned rd,unsigned rn,unsigned offset) {
    lagi_arm_word(w,0xe5900000u | rn<<16 | rd<<12 | offset);
}
static void lagi_arm_store(LagiDspArmWriter *w,unsigned rd,unsigned rn,unsigned offset) {
    lagi_arm_word(w,0xe5800000u | rn<<16 | rd<<12 | offset);
}
static void lagi_arm_half(LagiDspArmWriter *w,unsigned rd,unsigned rn,unsigned offset,int sign,int store) {
    if (offset>255) {
        /* r2 is always address scratch. Source/destination must not be r2. */
        lagi_arm_imm(w,2,offset); lagi_arm_dp(w,4,2,rn,2,0,0); rn=2; offset=0;
    }
    lagi_arm_word(w,(store?0xe1c000b0u:(sign?0xe1d000f0u:0xe1d000b0u)) |
        rn<<16 | rd<<12 | (offset&0xf0u)<<4 | (offset&15u));
}
static void lagi_arm_call(LagiDspArmWriter *w,uintptr_t target) {
    lagi_arm_imm(w,12,(UINT32)target); lagi_arm_word(w,0xe12fff3cu); /* blx r12, preserves Thumb tag */
}
static void lagi_arm_sign24(LagiDspArmWriter *w,unsigned reg) {
    lagi_arm_mov(w,reg,reg,0,8); lagi_arm_mov(w,reg,reg,2,8);
}
static void lagi_arm_lowbits(LagiDspArmWriter *w,unsigned rd,unsigned rm,unsigned bits) {
    lagi_arm_mov(w,rd,rm,0,32-bits); lagi_arm_mov(w,rd,rd,1,32-bits);
}
static void lagi_arm_temp(LagiDspArmWriter *w,unsigned rd,unsigned index,int store) {
    lagi_arm_load(w,3,13,16); /* sample-local DEC */
    lagi_arm_small(w,4,3,3,index); lagi_arm_small(w,0,3,3,127);
    lagi_arm_imm(w,2,offsetof(struct _SCSPDSP,TEMP)); lagi_arm_dp(w,4,2,4,2,0,0);
    lagi_arm_word(w,(store?0xe7800103u:0xe7900103u) | 2u<<16 | rd<<12); /* [r2,r3,lsl #2] */
    if (!store) lagi_arm_sign24(w,rd);
}
static inline INT32 lagi_dsp_native_sat(INT32 acc,unsigned twice) {
    /* Saturating the doubled value to 32 bits before 24 bits is equivalent
     * to an INT64 double then sat24, including INT32_MIN/MAX. */
    return __ssat(twice ? __qadd(acc,acc) : acc,24);
}
static __attribute__((noinline)) void lagi_dsp_native_begin(struct _SCSPDSP *DSP) {
    memset(DSP->EFREG,0,sizeof(DSP->EFREG));
}
static __attribute__((noinline)) void lagi_dsp_native_end(struct _SCSPDSP *DSP) {
    --DSP->DEC; memset(DSP->MIXS,0,sizeof(DSP->MIXS));
}
static unsigned lagi_dsp_emit_arm(const LagiDspProgram *program,UINT32 *code,unsigned capacity) {
    LagiDspArmWriter writer={code,0,capacity,0}, *w=&writer;
    unsigned i;
    /* r4 DSP, r5 ACC, r6 INPUTS, r7 MEMVAL, r8 FRC, r9 Y, r10 ADRS,
     * r11 SHIFTED. Push ten registers; 32-byte frame keeps SP 8-byte aligned.
     * Stack: B, X, Y, RAM pointer, DEC, ring mask, RBP, padding. */
    lagi_arm_word(w,0xe92d5ff0u); /* push {r4-r12,lr} */
    lagi_arm_small(w,2,13,13,32);
    lagi_arm_mov(w,4,0,0,0);
    lagi_arm_call(w,(uintptr_t)lagi_dsp_native_begin);
    for (i=5;i<=11;++i) lagi_arm_imm(w,i,0);
    lagi_arm_load(w,0,4,offsetof(struct _SCSPDSP,DEC)); lagi_arm_store(w,0,13,16);
    lagi_arm_load(w,0,4,offsetof(struct _SCSPDSP,RBL)); lagi_arm_small(w,2,0,0,1); lagi_arm_store(w,0,13,20);
    lagi_arm_load(w,0,4,offsetof(struct _SCSPDSP,RBP)); lagi_arm_mov(w,0,0,0,12); lagi_arm_store(w,0,13,24);
    for (i=0;i<program->steps;++i) {
        const LagiScspDspOp *op=&program->ops[i];
        if (op->needsInput) {
            if (op->IRA<32) lagi_arm_load(w,6,4,offsetof(struct _SCSPDSP,MEMS)+op->IRA*4);
            else if (op->IRA<48) { lagi_arm_load(w,6,4,offsetof(struct _SCSPDSP,MIXS)+(op->IRA-32)*4); lagi_arm_mov(w,6,6,0,4); }
            else lagi_arm_imm(w,6,0);
            lagi_arm_sign24(w,6);
        }
        if (op->IWT) {
            lagi_arm_store(w,7,4,offsetof(struct _SCSPDSP,MEMS)+op->IWA*4);
            if (op->needsInput && op->IRA==op->IWA) lagi_arm_mov(w,6,7,0,0);
        }
        if (op->needsTemp) lagi_arm_temp(w,1,op->TRA,0);
        if (op->ZERO) lagi_arm_imm(w,0,0);
        else lagi_arm_mov(w,0,op->BSEL?5:1,0,0);
        if (!op->ZERO && op->NEGB) lagi_arm_small(w,3,0,0,0);
        lagi_arm_store(w,0,13,0);
        if (op->multiply) {
            lagi_arm_store(w,op->XSEL?6:1,13,4);
            if (op->YSEL==0) lagi_arm_mov(w,0,8,0,0);
            else if (op->YSEL==1) { lagi_arm_half(w,0,4,offsetof(struct _SCSPDSP,COEF)+op->COEF*2,1,0); lagi_arm_mov(w,0,0,2,3); }
            else if (op->YSEL==2) { lagi_arm_mov(w,0,9,2,11); lagi_arm_lowbits(w,0,0,13); }
            else { lagi_arm_mov(w,0,9,2,4); lagi_arm_lowbits(w,0,0,12); }
            lagi_arm_mov(w,0,0,0,19); lagi_arm_mov(w,0,0,2,19);
            lagi_arm_store(w,0,13,8);
        }
        if (op->YRL) lagi_arm_mov(w,9,6,0,0);
        if (op->needsShift) {
            if (op->SHIFT<=1) {
                /* Encodings checked against the Vita assembler. QADD avoids
                 * overflowing a plain LSL for full-width ACC before SSAT. */
                if(op->SHIFT) {
                    lagi_arm_word(w,0xe105b055u); /* qadd r11,r5,r5 */
                    lagi_arm_word(w,0xe6b7b01bu); /* ssat r11,#24,r11 */
                } else lagi_arm_word(w,0xe6b7b015u); /* ssat r11,#24,r5 */
            } else {
                lagi_arm_mov(w,11,5,0,op->SHIFT==2?9:8); lagi_arm_mov(w,11,11,2,8);
            }
        }
        if (op->multiply) {
            lagi_arm_load(w,2,13,4); lagi_arm_load(w,3,13,8);
            lagi_arm_word(w,0xe0c10392u); /* smull r0,r1,r2,r3 */
            lagi_arm_mov(w,0,0,1,12); lagi_arm_dp(w,12,0,0,1,0,20);
            lagi_arm_load(w,1,13,0); lagi_arm_dp(w,4,5,0,1,0,0);
        } else lagi_arm_load(w,5,13,0);
        if (op->TWT) lagi_arm_temp(w,11,op->TWA,1);
        if (op->FRCL) {
            if (op->SHIFT==3) lagi_arm_lowbits(w,8,11,12);
            else { lagi_arm_mov(w,8,11,2,11); lagi_arm_lowbits(w,8,8,13); }
        }
        if (op->odd && (op->MRD || op->MWT)) {
            lagi_arm_half(w,0,4,offsetof(struct _SCSPDSP,MADRS)+op->MASA*2,0,0);
            if (!op->TABLE) { lagi_arm_load(w,1,13,16); lagi_arm_dp(w,4,0,0,1,0,0); }
            if (op->ADREB && program->adrsLive) { lagi_arm_lowbits(w,1,10,12); lagi_arm_dp(w,4,0,0,1,0,0); }
            if (op->NXADR) lagi_arm_small(w,4,0,0,1);
            if (op->TABLE) lagi_arm_lowbits(w,0,0,16);
            else { lagi_arm_load(w,1,13,20); lagi_arm_dp(w,0,0,0,1,0,0); }
            lagi_arm_load(w,1,13,24); lagi_arm_dp(w,4,0,0,1,0,0);
            lagi_arm_load(w,2,4,offsetof(struct _SCSPDSP,SCSPRAM)); lagi_arm_dp(w,4,2,2,0,0,1);
            lagi_arm_store(w,2,13,12);
            if (op->MRD) {
                lagi_arm_half(w,0,2,0,0,0);
                if (op->NOFL) lagi_arm_mov(w,0,0,0,8);
                else lagi_arm_call(w,(uintptr_t)UNPACK);
                lagi_arm_mov(w,7,0,0,0);
            }
            if (op->MWT) {
                lagi_arm_mov(w,0,11,op->NOFL?2:0,op->NOFL?8:0);
                if (!op->NOFL) lagi_arm_call(w,(uintptr_t)PACK);
                lagi_arm_load(w,2,13,12); lagi_arm_half(w,0,2,0,0,1);
            }
        }
        if (op->ADRL) {
            if (op->SHIFT==3) { lagi_arm_mov(w,10,11,2,12); lagi_arm_lowbits(w,10,10,12); }
            else lagi_arm_mov(w,10,6,2,16);
        }
        if (op->EWT) {
            lagi_arm_half(w,1,4,offsetof(struct _SCSPDSP,EFREG)+op->EWA*2,1,0);
            lagi_arm_mov(w,0,11,2,8); lagi_arm_dp(w,4,0,0,1,0,0);
            lagi_arm_half(w,0,4,offsetof(struct _SCSPDSP,EFREG)+op->EWA*2,0,1);
        }
    }
    lagi_arm_mov(w,0,4,0,0); lagi_arm_call(w,(uintptr_t)lagi_dsp_native_end);
    lagi_arm_small(w,4,13,13,32); lagi_arm_word(w,0xe8bd9ff0u); /* pop {r4-r12,pc} */
    return w->failed ? 0 : w->used*4;
}

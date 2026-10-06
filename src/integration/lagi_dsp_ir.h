#ifndef LAGI_DSP_IR_H
#define LAGI_DSP_IR_H
#include <stdint.h>
#include <string.h>
#include "lagi_dsp_platform.h"
/* Explicit controls describe ordered input/write, B/X/Y, previous-ACC shift,
 * multiply/add, TEMP/FRC, memory read-before-write, ADRS, and effect stages. */
typedef struct LagiScspDspOp {
#define LAGI_DSP_FIELD(name, word, shift, mask) uint8_t name;
#include "lagi_dsp_fields.def"
#undef LAGI_DSP_FIELD
    uint8_t odd, multiply, needsInput, needsTemp, needsShift;
} LagiScspDspOp;
typedef struct LagiDspProgram {
    uint16_t words[512];
    LagiScspDspOp ops[128];
    uint32_t hash;
    unsigned steps;
    unsigned frcLive, yLive, adrsLive;
} LagiDspProgram;
static inline int lagi_dsp_decode_ir(LagiDspProgram *program, const uint16_t *words, unsigned steps) {
    unsigned i;
    if (steps > 128) return 0;
    memset(program, 0, sizeof(*program));
    memcpy(program->words, words, steps*8);
    program->steps=steps; program->hash=lagi_dsp_program_hash(words,steps);
    for (i=0;i<steps;++i) {
        const uint16_t *p=words+i*4;
        LagiScspDspOp *op=&program->ops[i];
#define LAGI_DSP_FIELD(name, word, shift, mask) op->name=(p[word] >> shift)&mask;
#include "lagi_dsp_fields.def"
#undef LAGI_DSP_FIELD
        op->odd=i&1;
        if (op->IRA >= 0x32) return 0; /* Same valid domain as the reference. */
        program->frcLive |= op->FRCL; program->yLive |= op->YRL; program->adrsLive |= op->ADRL;
    }
    for (i=0;i<steps;++i) {
        LagiScspDspOp *op=&program->ops[i];
        op->multiply = op->YSEL==1 || (op->YSEL==0 ? program->frcLive : program->yLive);
        op->needsInput=(op->XSEL && op->multiply) || op->YRL || (op->ADRL && op->SHIFT!=3);
        op->needsTemp=(!op->ZERO && !op->BSEL) || (!op->XSEL && op->multiply);
        op->needsShift=op->TWT || op->FRCL || (op->ADRL && op->SHIFT==3) || op->EWT || (op->odd && op->MWT);
    }
    return 1;
}
#endif

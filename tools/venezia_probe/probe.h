#ifndef LAGI_VNZ_PROBE_H
#define LAGI_VNZ_PROBE_H
#include <stdint.h>
#define PROBE_MAGIC 0x4c565a50u
#define PROBE_ABI 1u
#define PROBE_MAX_ROUNDS 65536u
#define PROBE_UNTOUCHED 0xffffffffu
enum { PROBE_ECHO=1, PROBE_MATH=2 };
enum { PROBE_OK=0, PROBE_BAD_COMMAND=1, PROBE_BAD_ROUNDS=2 };
typedef struct {
    uint32_t magic, abi, command, serial, seed, rounds;
    uint32_t result, status, response_serial, guard;
} ProbePacket;
typedef char ProbePacketSizeCheck[sizeof(ProbePacket)==40 ? 1 : -1];
static inline uint32_t probe_math(uint32_t x, uint32_t rounds) {
    for(uint32_t i=0;i<rounds;++i) {
        x ^= x << 13; x ^= x >> 17; x ^= x << 5;
        x += i * 0x9e3779b9u;
    }
    return x;
}
static inline int probe_handle(ProbePacket *p, unsigned size) {
    if(!p || size!=sizeof(*p) || p->magic!=PROBE_MAGIC || p->abi!=PROBE_ABI)return -1;
    p->response_serial=p->serial;
    p->status=PROBE_OK;
    if(p->command==PROBE_ECHO) p->result=p->seed;
    else if(p->command!=PROBE_MATH) p->status=PROBE_BAD_COMMAND;
    else if(p->rounds>PROBE_MAX_ROUNDS) p->status=PROBE_BAD_ROUNDS;
    else p->result=probe_math(p->seed,p->rounds);
    return 0;
}
#endif

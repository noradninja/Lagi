#ifndef LAGI_VNZ_ELFCHECK_H
#define LAGI_VNZ_ELFCHECK_H
#include <stddef.h>
#include <stdint.h>
#define PROBE_ELF_LIMIT (256u*1024u)
static inline uint32_t probe_u16(const unsigned char *p) {
    return (uint32_t)p[0] | (uint32_t)p[1]<<8;
}
static inline uint32_t probe_u32(const unsigned char *p) {
    return probe_u16(p) | probe_u16(p+2)<<16;
}
static inline int probe_span(size_t n,uint32_t o,uint32_t s) {
    return o<=n && s<=n-o;
}
static inline int probe_comm_overlap(uint32_t o,uint32_t s) {
    return s && o<768 && (uint64_t)o+s>256;
}
/* Runner executes file bytes in place and writes a 512-byte mailbox at 256.
 * Reject malformed/unsupported layout before invoking firmware code. */
static inline int probe_elf_check(const unsigned char *p,size_t n) {
    if(!p || n<768 || n>PROBE_ELF_LIMIT || p[0]!=0x7f || p[1]!='E' ||
        p[2]!='L'||p[3]!='F'||p[4]!=1||p[5]!=1||p[6]!=1||
        probe_u16(p+16)!=2||probe_u16(p+18)!=0xf00d||probe_u32(p+20)!=1||
        probe_u16(p+40)!=52) return 0;
    const uint32_t entry=probe_u32(p+24),phoff=probe_u32(p+28),shoff=probe_u32(p+32);
    const uint32_t phsz=probe_u16(p+42),phn=probe_u16(p+44),shsz=probe_u16(p+46),shn=probe_u16(p+48);
    if(!shn || shsz!=40 || !probe_span(n,shoff,shsz*shn) ||
       probe_comm_overlap(shoff,shsz*shn) ||
       (phn && (phsz!=32 || !probe_span(n,phoff,phsz*phn) || probe_comm_overlap(phoff,phsz*phn))))return 0;
    int entry_ok=0;
    for(uint32_t i=0;i<shn;++i){
        const unsigned char *s=p+shoff+i*40;
        const uint32_t type=probe_u32(s+4),flags=probe_u32(s+8),addr=probe_u32(s+12),off=probe_u32(s+16),size=probe_u32(s+20);
        if(type==8 && size && (flags&2))return 0; /* no BSS in this probe */
        if(type!=8 && (!probe_span(n,off,size)||probe_comm_overlap(off,size)))return 0;
        if(type==1 && (flags&6)==6 && addr==off && entry>=addr && entry-addr<size)entry_ok=1;
    }
    return entry_ok && entry>=768 && !(entry&1);
}
#endif

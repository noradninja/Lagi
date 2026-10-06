/*
 * Lagi-owned SCSP mixer specialization.
 *
 * Keep the complete upstream implementation in this translation unit, but
 * replace only its public per-sample entry point. Unsupported slot modes use
 * the original SCSP_UpdateSlot() unchanged.
 */
#define SCSP_Update SCSP_Update_Upstream
#include "../../extern/Azel/ThirdParty/aosdk/eng_ssf/scsp.c"
#undef SCSP_Update

extern volatile unsigned lagi_dsp_profile_sample;

volatile unsigned lagi_scsp_profile_fast_slots_last = 0;
volatile unsigned lagi_scsp_profile_generic_slots_last = 0;

#define LAGI_SCSP_ALWAYS_INLINE \
    static inline __attribute__((always_inline))

struct lagi_scsp_slot_mix_cache
{
    UINT16 reg6;
    UINT16 rega;
    UINT16 regb;
    INT32 ring_gain;
    INT32 dsp_gain;
    INT32 left_gain;
    INT32 right_gain;
    UINT8 isel;
    UINT8 valid;
};

static struct lagi_scsp_slot_mix_cache g_lagiSlotMixCache[32];

LAGI_SCSP_ALWAYS_INLINE struct lagi_scsp_slot_mix_cache *
lagi_scsp_mix_cache(
    struct _SCSP *scsp,
    struct _SLOT *slot,
    int slot_index)
{
    struct lagi_scsp_slot_mix_cache *cache =
        &g_lagiSlotMixCache[slot_index];
    const UINT16 reg6 = slot->udata.data[0x6];
    const UINT16 rega = slot->udata.data[0xA];
    const UINT16 regb = slot->udata.data[0xB];

    if (!cache->valid || cache->reg6 != reg6 || cache->rega != rega ||
        cache->regb != regb)
    {
        const unsigned tl = TL(slot);
        unsigned enc;

        cache->reg6 = reg6;
        cache->rega = rega;
        cache->regb = regb;
        cache->isel = (UINT8)ISEL(slot);

        enc = tl | (0x7u << 13);
        cache->ring_gain = scsp->LPANTABLE[enc];
        enc = tl | ((unsigned)IMXL(slot) << 13);
        cache->dsp_gain = scsp->LPANTABLE[enc];
        enc = tl | ((unsigned)DIPAN(slot) << 8) |
            ((unsigned)DISDL(slot) << 13);
        cache->left_gain = scsp->LPANTABLE[enc];
        cache->right_gain = scsp->RPANTABLE[enc];
        cache->valid = 1;
    }

    return cache;
}

LAGI_SCSP_ALWAYS_INLINE int lagi_scsp_plain_pcm_supported(
    struct _SLOT *slot)
{
    const unsigned loop_mode = LPCTL(slot);

    return SSCTL(slot) == 0 && PLFOS(slot) == 0 && ALFOS(slot) == 0 &&
        MDL(slot) == 0 && MDXSL(slot) == 0 && MDYSL(slot) == 0 &&
        !slot->Backwards && loop_mode <= 1;
}

LAGI_SCSP_ALWAYS_INLINE void lagi_scsp_advance_plain_slot(
    struct _SLOT *slot)
{
    UINT32 addr1;
    UINT32 addr2;
    const UINT32 lsa = LSA(slot);
    const UINT32 lea = LEA(slot);

    slot->cur_addr += slot->step;
    slot->nxt_addr = slot->cur_addr + (1u << SHIFT);
    addr1 = slot->cur_addr >> SHIFT;
    addr2 = slot->nxt_addr >> SHIFT;

    if (addr1 >= lsa && LPSLNK(slot) && slot->EG.state == ATTACK)
        slot->EG.state = DECAY1;

    if (LPCTL(slot) == 0)
    {
        if (addr1 >= lsa && addr1 >= lea)
            SCSP_StopSlot(slot, 0);
        if (addr2 >= lsa && addr2 >= lea)
            SCSP_StopSlot(slot, 0);
    }
    else
    {
        if (addr1 >= lea)
        {
            const UINT32 remainder =
                slot->cur_addr - (lea << SHIFT);
            slot->cur_addr = (lsa << SHIFT) + remainder;
        }
        if (addr2 >= lea)
        {
            const UINT32 remainder =
                slot->nxt_addr - (lea << SHIFT);
            slot->nxt_addr = (lsa << SHIFT) + remainder;
        }
    }
}

LAGI_SCSP_ALWAYS_INLINE INT32 lagi_scsp_finish_plain_slot(
    struct _SLOT *slot,
    struct lagi_scsp_slot_mix_cache *cache,
    INT32 sample)
{
    if (SBCTL(slot) & 0x1)
        sample ^= 0x7FFF;
    if (SBCTL(slot) & 0x2)
        sample = (INT16)(sample ^ 0x8000);

    lagi_scsp_advance_plain_slot(slot);

    if (slot->EG.state == ATTACK)
        sample = (sample * EG_Update(slot)) >> SHIFT;
    else
        sample =
            (sample * EG_TABLE[EG_Update(slot) >> (SHIFT - 10)]) >> SHIFT;

    if (!STWINH(slot))
        *RBUFDST = (sample * cache->ring_gain) >> (SHIFT + 1);

    return sample;
}

LAGI_SCSP_ALWAYS_INLINE INT32 lagi_scsp_update_plain_pcm8(
    struct _SCSP *scsp,
    struct _SLOT *slot,
    struct lagi_scsp_slot_mix_cache *cache)
{
    const UINT32 base = SA(slot);
    const UINT32 addr1 = slot->cur_addr >> SHIFT;
    const UINT32 addr2 = slot->nxt_addr >> SHIFT;
    const INT32 sample1 =
        (INT32)(INT8)scsp->SCSPRAM[(base + addr1) & 0x7FFFFu] * 256;
    const INT32 sample2 =
        (INT32)(INT8)scsp->SCSPRAM[(base + addr2) & 0x7FFFFu] * 256;
    const INT32 fraction =
        (INT32)(slot->cur_addr & ((1u << SHIFT) - 1u));
    const INT32 interpolated =
        sample1 * ((1 << SHIFT) - fraction) + sample2 * fraction;

    return lagi_scsp_finish_plain_slot(
        slot, cache, interpolated >> SHIFT);
}

LAGI_SCSP_ALWAYS_INLINE INT32 lagi_scsp_update_plain_pcm16(
    struct _SCSP *scsp,
    struct _SLOT *slot,
    struct lagi_scsp_slot_mix_cache *cache)
{
    const UINT32 base = SA(slot);
    const UINT32 addr1 = (slot->cur_addr >> (SHIFT - 1)) & 0x7FFFEu;
    const UINT32 addr2 = (slot->nxt_addr >> (SHIFT - 1)) & 0x7FFFEu;
    const UINT8 *source1 =
        scsp->SCSPRAM + ((base + addr1) & 0x7FFFEu);
    const UINT8 *source2 =
        scsp->SCSPRAM + ((base + addr2) & 0x7FFFEu);
    const INT32 sample1 =
        (INT16)(((UINT16)source1[0] << 8) | source1[1]);
    const INT32 sample2 =
        (INT16)(((UINT16)source2[0] << 8) | source2[1]);
    const INT32 fraction =
        (INT32)(slot->cur_addr & ((1u << SHIFT) - 1u));
    const INT32 interpolated =
        sample1 * ((1 << SHIFT) - fraction) + sample2 * fraction;

    return lagi_scsp_finish_plain_slot(
        slot, cache, interpolated >> SHIFT);
}

static void lagi_scsp_do_master_sample(
    struct _SCSP *scsp,
    stereo_sample_t *output)
{
    const int profile_sample = lagi_dsp_profile_sample != 0;
    unsigned fast_slots = 0;
    unsigned generic_slots = 0;
    INT32 left = 0;
    INT32 right = 0;
    int slot_index;
    int effect_index;

    for (slot_index = 0; slot_index < 32; ++slot_index)
    {
        struct _SLOT *slot = &scsp->Slots[slot_index];

#if FM_DELAY
        RBUFDST = scsp->DELAYBUF + scsp->DELAYPTR;
#else
        RBUFDST = scsp->RINGBUF + scsp->BUFPTR;
#endif

        if (slot->active)
        {
            struct lagi_scsp_slot_mix_cache *cache =
                lagi_scsp_mix_cache(scsp, slot, slot_index);
            INT32 slot_sample;

            if (lagi_scsp_plain_pcm_supported(slot))
            {
                if (PCM8B(slot))
                    slot_sample = lagi_scsp_update_plain_pcm8(
                        scsp, slot, cache);
                else
                    slot_sample = lagi_scsp_update_plain_pcm16(
                        scsp, slot, cache);
                if (profile_sample)
                    ++fast_slots;
            }
            else
            {
                slot_sample = SCSP_UpdateSlot(scsp, slot);
                if (profile_sample)
                    ++generic_slots;
            }

            scsp->DSP.MIXS[cache->isel] +=
                (slot_sample * cache->dsp_gain) >> (SHIFT - 2);
            left += (slot_sample * cache->left_gain) >> SHIFT;
            right += (slot_sample * cache->right_gain) >> SHIFT;
        }
        else
        {
            g_lagiSlotMixCache[slot_index].valid = 0;
        }

#if FM_DELAY
        scsp->RINGBUF[(scsp->BUFPTR + 64 - (FM_DELAY - 1)) & 63] =
            scsp->DELAYBUF[
                (scsp->DELAYPTR + FM_DELAY - (FM_DELAY - 1)) % FM_DELAY];
#endif
        ++scsp->BUFPTR;
        scsp->BUFPTR &= 63;
#if FM_DELAY
        ++scsp->DELAYPTR;
        if (scsp->DELAYPTR > FM_DELAY - 1)
            scsp->DELAYPTR = 0;
#endif
    }

    SCSPDSP_Step(&scsp->DSP);

    for (effect_index = 0; effect_index < 16; ++effect_index)
    {
        struct _SLOT *slot = &scsp->Slots[effect_index];
        if (EFSDL(slot))
        {
            const unsigned enc =
                ((unsigned)EFPAN(slot) << 8) |
                ((unsigned)EFSDL(slot) << 13);
            left +=
                (scsp->DSP.EFREG[effect_index] * scsp->LPANTABLE[enc]) >>
                SHIFT;
            right +=
                (scsp->DSP.EFREG[effect_index] * scsp->RPANTABLE[enc]) >>
                SHIFT;
        }
    }

    output->l = ICLIP16(left >> 2);
    output->r = ICLIP16(right >> 2);

    SCSP_TimersAddTicks(scsp, 1);
    CheckPendingIRQ(scsp);

    if (profile_sample)
    {
        lagi_scsp_profile_fast_slots_last = fast_slots;
        lagi_scsp_profile_generic_slots_last = generic_slots;
    }
}

void SCSP_Update(void *param, INT16 **inputs, stereo_sample_t *sample)
{
    (void)param;
    (void)inputs;
    if (lagi_dsp_profile_sample)
    {
        lagi_scsp_profile_fast_slots_last = 0;
        lagi_scsp_profile_generic_slots_last = 0;
    }
    lagi_scsp_do_master_sample(&SCSP, sample);
}

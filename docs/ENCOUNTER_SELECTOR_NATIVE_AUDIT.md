# Native encounter story selection audit

## Evidence and correction

Original US Disc 1 MAIN, file LBA 58, base 06006000:

- 0600C720..0600C730 loads COMMON entry 00208710 + fieldIndex*20.
- 0600C732..0600C748 reads the entry's story bit and its MSB-first byte mask.
- 0600C74A..0600C752: TST R2,R2; MOVT R0; XOR #1,R0; SHLL2; SHLL.
  TST sets T for a **zero** masked byte. The result after XOR is therefore
  zero for a clear bit, one for a set bit, then multiplied by eight.
- 0600C754..0600C75E adds entry+4 and stores the selector/overlay.
- Literal 0600C918 is 00208710; 0600C920 is the native MSB-first mask table
  at 06042F14 (80,40,20,10,08,04,02,01).

Azel's common.h getBit returns true for set bits. The reconstruction's
`(getBit(bitIndex)^1)*8` in encounterTaskUpdate reversed this native sense.
The CMake-generated adapter replaces it with `(getBit(bitIndex)?8:0)` using
an exact source-boundary guard. extern/Azel is unchanged. No field/story-bit
ID filter, chance tuning, extra random call or wall-clock counter is added.

Original COMMON at LBA 182, field 2 entry 00208738, bit 0057h:

| Bit | Selector | Root | Initial | Reset | Failure cap |
|---|---|---|---|---|---|
| Clear | 0020873C | 00207624 | 90 | 120 | 10 |
| Set | 00208744 | 002076A0 | 120 | 150 | 20 |

The recent hardware log selects 00208744 while reporting value=0, proving
the previous generated code followed the reversed branch in that run. This
does not establish encounter timing after the correction. The selector is
initialized by task status zero; there is no new per-frame story reselect.

## Checks and limits

encounter_selector_test extracts the actual generated initializer and actual
common.h getBit. It checks the complete original selector instruction prefix,
condition, suffix, literals and mask bytes, then executes the five-instruction
condition independently. Both real field-2 branches and their native metadata
are compared against production output. Synthetic header variations cover all
5040 bit positions and unrelated set bits. This is not a general SH2 emulator.

Existing encounter_rng_test separately checks original seed/RNG/countdown/
probability instructions, COMMON weights and a million generated RNG outputs.
encounter_trace_test checks observer neutrality for area/rate evaluation.
No named formation is guaranteed by these fixtures. Full native scheduler
timing and corrected hardware encounter behavior are separate acceptance gates.

Hardware should now report selector=0020873C, root=00207624 when field=2,
storyBit=87, value=0. Verify normal encounters, danger display with a Telepathy
Shard, repeated/subfield transitions and return from battle. A set bit must
instead select 00208744; do not alter save flags merely to force a test.

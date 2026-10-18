# Gauge-charge arithmetic crash, October 18

The supplied `psp2core-1792307071-0x00290232af-eboot.bin.psp2dmp`
expands to 5,865,504 bytes; all 64 ELF segments fit. Module text is loaded
at 81039000. Main-thread PC 812266A2 is text offset 1ED6A2, the abort trap.
The preserved assertion line is 334. The supplied matching Vita ELF identifies
the caller at offset B34D6 as the zero-divisor assertion in `performModulo`;
stack offsets 12E5DA/12E7FE identify charge-effect creation/root update.
Disassembly of that ELF confirms size >> 2 feeds the remainder divisor.
This is not evidence of a waterfall particle lifetime crash.

The native BTL_A3 creator 06085080 calls MAIN 06035550 with base size >> 2
and a random draw. MAIN 06035550 tests a zero divisor, branches to 060355F0,
returns zero and stores arithmetic error 044E at 06053254. The corresponding
other remainder entry's zero branch at 0603553A also returns zero. The C++
reconstruction replaced that behavior with assertions.

The generated-source adapter now returns zero in both remainder services for
this case and reports `[NativeArithmetic] ... error=044E divisor=0 result=0`
at most eight times per helper. The native absolute-address error storage has
no mapped C++ owner here: the diagnostic reports it rather than writing an
invented address. Nonzero-divisor behavior is unchanged. RNG draws, charge
parameters, allocation and effect lifecycle remain unchanged; no broad assert
disable or guessed minimum charge size is introduced.

`tests/native_remainder_test.ps1` extracts the actual generated helpers and
checks zero/nonzero divisors, unsigned boundaries and bounded diagnostics.
Both battle-enabled and normal fullres/dither Vita VPK builds pass. The
on-device recurrence test (charge then immediately spend gauges, repeated
battles) remains pending. This addresses this assertion, not every battle crash.

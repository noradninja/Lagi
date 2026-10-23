# Tunnel exit crash and audio investigation

The latest fatal entry is a C assertion of `divisor` in setDividend, on the
main thread (40010003), not an SCSP worker failure. Matching E:/dev/Lagi
build-battle-vita/lagi symbols put __wrap_abort at 811EA7E6; the logged
812587E6 gives relocation +6E000. Caller 8110D482 resolves to setDividend;
caller 81195030 resolves to LCSTaskDrawSub4, followed by lock-on task drawing.
This is a camera-plane projection with denominator zero. Older fatal entries
in the append-only file are not this crash.

MAIN 06035686 uses DMULS.L and the SH7604 signed 64/32 division peripheral,
then returns its quotient register. The reconstruction instead asserts on
zero and narrows an overflowing quotient. The generated Vita implementation
now returns saturated quotient results for zero/overflow and retains signed
truncating arithmetic for ordinary input. Zero-divisor sign follows the
64-bit product, including positive saturation for a zero product. No LCS
target suppression, camera change, or tunnel-specific exception is added.
The helper does not emulate peripheral remainder/status/interrupt registers:
this engine API returns only the quotient.

tests/saturn_divide_test.ps1 checks the original Disc 1 instruction stream
and production arithmetic, signed boundaries, zero and camera-plane crossings.
These checks do not prove the on-device crash is resolved.

The same run contains no BattleStartup encounter and supplies no new combat
timing acceptance. Audio quanta near its end exceed the 5804-us budget with
DSP backend 0, so the music slowdown is not evidence that the main-thread
assertion occurred inside audio. That independent performance issue remains
under investigation.

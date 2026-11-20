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

## Tunnel music stop

Authored cave entries already call soundFunc(100) and soundFunc(101). The
reconstruction's dummy.cpp implements soundFunc as a no-op. FLD_A3's
subfield dispatch at 06081C2C selects tunnel 4 entry 06054718; calls at
0605472E/06054734 use literal 060547A8 = 0602C32A for IDs 100/101.
MAIN 0602C32A sets R7=0, R6=0, R5=2, and tail-calls the enqueue service at
0602B188. The generated adapter now supplies exactly that shared stop request.
No tunnel-ID mute list or extra bank reload is added. Native same-sequence
load state 8 remains valid: silence no longer depends on a bank-change stop.
The existing queued-request service delivers the authored stops after an
in-progress load, preserving its worker ownership and command ordering.

Hardware checks required: first and subsequent tunnel silence, outside BGM
restart, tunnel ambience/switch sounds, and the reported exit route/crash.
Do not call these source/build checks device acceptance.

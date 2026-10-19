# E011 transition abort: 2026-10-19

Priority: Deep Canyon Gulch exit before the Arachnoth cinematic. No visual
changes, callback bypasses or claimed crash fix in this diagnostic checkpoint.

## Exact supplied evidence

| File | SHA256 |
| --- | --- |
| psp2core-1792419594-0x002c092ebd-eboot.bin.psp2dmp.tmp | `E92D555CA1F9D6F3FCD33D503E8FC47B7EDFD1CBC28C0DE0EA7A14B4ABF62CA0` |
| lagi.velf | `9D3FB8ED7CC8F891AC502E647E830B1638DEB5FEB69A9AA3074488A0BADCD30C` |
| lagi.log | `4B8869AFF5B3B1777D9965A7261ECCC8F33447B58DC1067103D3C20C9B56233B` |

The partial core expands to 9,429 bytes. It declares 63 program headers, but
only eight complete note segments and part of PROCESS_INFO are present. There
is no stack/code/module memory. The complete THREAD_REG_INFO record retains
main PC `81227022`, LR `81226FEF`, SP `81B805A0`, r5/r6 zero and r7 `81B805C8`.

Matching executable bytes contain the `_kill_r` undefined-instruction stop at
`811E5022`, consistent with relocation `+42000`. The retained registers do not
carry `__assert_func`'s preserved file, line and expression arguments as in the
older E011 core. This proves a fatal abort path, not its original caller. A C++
exception, direct abort or earlier corruption cannot be distinguished by those
registers alone. Do not claim this is the old missing script assertion.

Sony conversion leaves a misleading symbol-table address domain in the VELF:
prefer the matching unconverted `lagi` for symbolic disassembly and verify actual
bytes in the VELF. In this build the unconverted ELF lists `_kill_r` at
`811E4FE0`; its trap is `+42`. Do not equate the nearest VELF nm value with the
function's instruction address without checking.

The normal log ends after E14 sound bank/sequence 68 readiness and has no EPK
startup entry in this run. It uses an asynchronous queue, so absence of later
messages does not establish exactly which operation aborted. E011 resource,
script, background, main logic, projection and camera initialization remain
candidate paths. Separately known missing scheduled callbacks remain unfinished;
they must not be wired to no-ops to make this failure disappear.

## Durable fatal reporting

Battle builds now wrap external references to `abort` and `__cxa_throw`. Before
forwarding unchanged arguments to the originals, they synchronously append to
`ux0:data/lagi/lagi-fatal.log` using a separate file descriptor, not the normal
log mutex or queue. They record fatal/throw kind, bounded exception type name,
thread, immediate caller, the runtime `__wrap_abort` anchor, last E011 startup
step and up to 32 unwind PCs. Exception records include handled throws too;
they are diagnostic evidence, not proof that each throw caused termination.

The caller/anchor are written before attempting unwinding. Missing unwind tables
can shorten the trace; Vita stack corruption can still prevent a report. The
reporter is allocation-free in its own code, recursion guarded, and never
swallows the abort or exception. A file-open failure does not change original
termination. There are no task-loop clock reads, per-frame logs, rendering or
gameplay changes. E011 stage markers only store static strings at startup.

`tests/fatal_trace_test.ps1` executes the production wrapper/reporting bodies
with mocked IO/unwinder and sentinel noreturn handlers. It covers append mode,
stage context, frame bounds, exception argument forwarding, original abort
forwarding and file-open failure. Vita compilation passes; linked disassembly
confirms the wrapper calls original `abort`. Hardware reporting and the actual
transition fix are still pending.

Next hardware evidence: reproduce the same exit and attach **lagi-fatal.log**,
ordinary lagi.log and the matching lagi.velf (partial core optional). The runtime
anchor permits relocation even if the next dump again omits MODULE_INFO.

## Complete follow-up core: proven missing scheduler and recovery

`psp2core-1792420901-0x000bb22e7d-eboot.bin.psp2dmp` expands to 5,789,144
bytes with all 56 segments present. Code base `8102B000`; PC `81210562` is
the fatal stop. The retained stack now identifies townScript's one-argument
dispatcher assertion, line 251, script location `060541F6`, native function
`0605890C`, and E011's live overlay owner. The fatal report's caller is
`8120A677`, imageAbort `811F0793`, last startup stage startCameraTask. This
confirms the known missing E011 scheduler. Do not continue treating the lack of
EPK messages or the older partial registers as proof of an earlier/different
failure: async log loss and incomplete state did not establish that distinction.

The generated adapter now registers `0605890C` and its native child callbacks:
quad `060589E2`, model `06058B2A`, ribbon `06058FF8`. Original table `0605FD10`
has 78 records, including four ribbons. Each record is **threshold, argument,
callback**; original `060588E2..E6` loads the function from +8 and the argument
from +4. The native scheduler samples the clock once per update, catches up all
due records in order, and retires at the end sentinel on the next update. All
78 original records are checked in place by the disc-backed fixture.

The ribbon task retains native resource ownership, its 20-position history,
one-time sound, sine-displaced flight, oriented random amplitude, 30-tick tail,
per-segment widths, sprite selectors and RGB555 gradient. Its world two-width
strip uses the mechanically extracted native fixed-point projector and existing
packet sender, with renderer depth metadata. It deliberately does not replace
the accepted rotated-square collection/charge ray service.

Three later unregistered calls were also recovered instead of waiting for the
next assertion: palette sequencing `0605809E`, wave control `060557E0`, and
point-tracking screen control `06055872`. They retain native clock, stop, bank,
loop/delay, graphics decompression, state and duration behavior. The tracking
point is converted from big-endian disc data into background-task-owned storage,
not aliased as a host Vec3. Overlapping persistent transfers are removed through
the typed transfer list rather than Saturn raw pointer offsets.

Host tests execute the production scheduler/ribbon and later callback bodies,
including catch-up, delta-zero, sound once, tail expiry, draw resource addresses,
color endpoints, palette delay/termination, screen mode changes, transfer
unlinking, owned point and cinematic stop. Separate existing leaf and 1,024-case
ray packet differential tests pass, as does the accepted collection-burst
regression. Original instructions and all scheduled callback kinds are checked
against the user's disc without committing assets. Both Vita build variants
compile. This removes the proven dispatcher failure; complete on-device E011
playback and handoff to the Arachnoth fight remain unverified.

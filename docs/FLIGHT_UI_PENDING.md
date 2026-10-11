# Current flight UI/transition acceptance

User confirms radar frame/orb positioning and drawing corrected after f3d0cd4.
Navigator release still crashes. Missing orientation line/object markers and
encounter-risk color behavior remain unaccepted. Tunnel exit can leave output
white while game/audio continues. These requirements remain in scope.

Dump 1791750699 is gzip-compressed and expands to a complete 5,858,872-byte ELF
core (all 64 declared segments fit). The available Downloads/lagi.log predates
the dump and ends in save-menu mode, so it is not matched navigator evidence.
Available local build is f3d0cd4. Raw Vita thread-register notes require verified
layout/base mapping before claiming a faulting PC or crash cause.

Source audit found field.cpp's exitCutsceneTaskUpdateSub0Sub1 constructor did
not set m14_status. Its state machine reads that scalar on the first update;
createSubTaskFromFunction uses plain new T, unlike the original cleared task
allocation. Generated code now initializes it to zero while retaining native
fade requests, wait, destination replacement and unlock order. Syntax/source
checks do not prove this resolves the white screen: collect a fresh tunnel
capture/log and inspect the incoming field's fade behavior if it persists.

No forced fade-in, no navigator disabling, and no invented radar markers are
added. Native destination indicator and palette-fade stubs still require actual
reconstruction before their visual behavior can be called restored.

Incoming-area mismatch: s_FieldSubTaskWorkArea::Update state 0 requested its
existing 30-frame fade only under !isSoundLoadingFinished(). The Vita audio
service returns true when ready, so completed audio prevented that fade. The
generated adapter uses the positive readiness gate (not a forced renderer
fade), while preserving the file-list wait, priority 6, channel targets
m_48/m_4A and one-shot state advance. Init now clears fieldSubTaskStatus under
the original cleared-task contract. Source/CPU checks are not native runtime
or hardware validation. Verify white -> load -> neutral on the same tunnel.

Orientation initializer audit: original FLD_A3 06071E3E stores m34=10
(06071E76/06071E8C), m28=+29 (06071E8E/06071E98), and initial
mC_X=m8+96 (06071E7C). Reconstructed source instead used 24, -29 and +48.
The equal outer/inner radii produced a zero-length direction line; its center
was also on the wrong side of the frame. Generated adapters restore these
three original values without modifying extern/Azel or the renderer's accepted
integer-grid placement. radar_direction_test.ps1 checks anchors, cardinal
segment geometry and patched-source Vita syntax. Rotation and on-device line
appearance remain unverified; destination dots/palette fade are not restored
by this initializer repair.

Hardware report on f1ca85c: tunnel exit now fades back into gameplay correctly.
Navigator release still fails, and save-file load shows garbled tiles instead
of the required black fade. These are separate acceptance gates.

New dump 1791751375 expands to 5,850,576 bytes; all 63 segments fit. MODULE_INFO
places lagi RX at 81048000, whereas the matching local raw ELF starts at
81000000: subtract 48000 before symbolizing RX addresses. PC 8119A91A maps to
_kill_r; the stack contains abort/__assert_func and return 8112ACB1 mapping to
FLD_A3_data::executeNative's default assert at raw 810E2CAC. Native dispatch
argument 06073172 appears on that stack. This proves a deliberate default
assert, not an arbitrary GPU fault; it does not explain the reported regression
timing by itself. Do not disable navigator or return a guessed constant.

Original 06073172 reads global pointer 060525D8, then pointer at +8, then its
first byte, and returns byte != 0. Main executable 060256A8's string-length
routine uses context at 060525D0 (literal 06025780); its +8 table member is
060525D8. That establishes the queried text as global string slot 2, not a
field task status byte. The generated dispatch now queries the existing
vdp2StringContext2 table via a read-only helper, returning the original boolean
for populated entries. Missing/unallocated entries safely return false.
There is no substitute table, forced result, or navigator disabling.
navigator_text_dispatch_test.ps1 verifies the unique patch anchor and compiles
the patched original dispatcher and VDP2 helper with the Vita compiler. These
are syntax checks only; release-R behavior requires a new device test. The
user's report that this worked before radar placement changes remains recorded;
the dump establishes the failing path, not that historical regression's cause.
Load-transition video and black fade still require separate diagnosis.

# Current flight UI/transition acceptance

Additional user confirmation: scene sprite depth is correct on hardware.
Retain the explicit producer-depth path; do not revert it while restoring
radar line state or native point lighting.

Hardware report on `cf82533`: user reports the combined fixes work except the
radar orientation line and navigator direction line. Preserve that acceptance
for navigator lifecycle, markers, scene depth and collection effects; it does
not establish completion of unfinished menu pages or native point lighting.
The renderer CMD6 branch lacked front/back line polygon mode, unlike CMD5.
It now selects LINE for CMD6 and restores TRIANGLE_FILL before subsequent
sprites; altitude polygons retain fill mode. Native endpoints/ownership/order
are unchanged. Source state-order guard and Vita syntax are the checks; actual
visibility of the two lines remains a device gate.

User confirms radar frame/orb positioning and drawing corrected after f3d0cd4,
and tunnel traversal/fade-in corrected after f1ca85c. Latest hardware 8b9cf45
still crashes on navigator release. Orientation/object markers, encounter-risk
color, scene sprite/mesh occlusion and collection radial burst remain visually
unaccepted. See NATIVE_FLIGHT_RESTORATION.md for the combined source repairs
and their actual checks; do not treat this historical audit as device PASS.

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

No forced fade-in, navigator disabling, or invented radar markers were added
at that checkpoint. Destination-indicator and palette-fade stubs then remained
unrestored. The combined restoration below supersedes the destination stub;
palette-fade and visual acceptance remain separate requirements.

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

Hardware 8b9cf45 still crashes on R-release. Dump 1791752192 is complete
(5,858,872 bytes, 64 segments); lagi RX base 8107F000 requires subtracting
7F000 from runtime RX addresses against the matching raw ELF. Stack return
81161D49 resolves to executeNative default assert at raw 810E2D48, now with
native argument 06073120 (not the previous 06073172). Thus the prior repair
did not complete navigator restoration. Original 06073120 obtains radar and
script tasks, checks script +44 multiChoiceData, then calls 0601DD66 with
parent script, output task +3C, result choice +4, radar filtered count +24,
radar choice-pointer array +20, per-choice color table, and selected index.
Existing updateMultiChoice has that seven-argument native role, but expects a
sSaturnPtr table rather than the reconstructed host s32* array. That ownership
and address translation must be repaired faithfully before wiring the call.
Do not merely add an empty case or suppress the default assertion.

Navigator continuation audit (user requests following the whole line before
another test): script 06080FAD..0608106E calls 060730BC (filter/build),
06073172 (selected-text branch), 06073120 (open dialog), and waits on
06072F3C (commit selection/free choice array). Both dialog branches contain
the open/wait calls. 06072F3C is also missing from the portable dispatcher.
Native 06072E78 maps filtered dialog index back to destination index; negative
means no change, exhausting destinations returns index 0 (deactivate).
06072EE8 updates global string slot 2 and selection, preserving cancellation.

Additional connected defects: createMultiChoice asserts on a nonnegative
default index. Original 06079234 stores that index, clears all color entries,
then 060792BA sets selected entry color to 9. Current parseDestinations leaves
entry 0's text as a null placeholder, and filtered selection text casts raw
Saturn addresses to host char pointers. A native address table cannot simply
be wrapped in a small memory-file view: readSaturnEA preserves the table's
m_file, so target strings must remain valid in that same view or be translated
through an explicit host-address dialog adapter. Audit allocation/free and
dialog lifetime for first use, reopen, selection, cancel and deactivate.

The nonnegative createMultiChoice default branch is now locally restored to
store the native index, clear styles, and mark the selected entry color 9.
Patched-source Vita syntax passed; it is uncommitted and not a standalone
hardware handoff. User additionally requires missing radar direction line and
item/object dots repaired before the next build. Treat initializer repair as
insufficient without auditing draw/submission and marker ownership routines.

Radar continuation audit: renderer CMD6 uses indices 0,1 and a draw count of
2; capture copies both endpoints and radar ownership, so the index buffer
does not explain the missing line. Direction trigonometry uses native 12-bit
table indices, not full shifted angles. The direction command is authored
between the orb and frame. These source checks are not visual acceptance.

Nearby object dots are a separate missing path, not just the stub destination
indicator at 06071EC4. Original LCSTaskDrawSub5 at 0606E750 calls 0607281E
through literal 0606E7E0 for each pending target and each active-list target.
The reconstructed LCSTaskDrawSub5 contains only cursor/selected-task drawing.
Native marker input is position, target flags & 3 as color index, and flags 1;
targets with flag 0100 first transform through the matrix at 0604B088.
0607281E performs strict X/Z range and squared horizontal distance checks,
rotates by negative dragon yaw, scales X by 0013CCCC and Z by FFEE0000
relative to radar encounter distance, and selects colors from 06093F54:
C3F0, C3FF, FE1F, FFF0. Vertical offsets above m5C/below -m60 add flags
4/8. Quad shaping is native 060726DC, followed by polygon emission at
0602CAF4. Object marker commands are emitted outside fieldRadar_draw's
current ownership range; restoration must extend explicit ownership rather
than infer HUD membership from colors, texture IDs or positions. Destination
indicator also calls this routine with color 4 and flags 12 after its bearing
line. No marker implementation or new hardware handoff is claimed yet.

The audit above preceded the combined implementation. Native navigator open,
wait/commit, address translation, cancel semantics and dialog-owned string view
are now restored, as are object-list markers and destination bearing/marker
emission. Read NATIVE_FLIGHT_RESTORATION.md for tests and device limitations.

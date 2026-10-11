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

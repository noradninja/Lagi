# Native WORLD handoff boundary (not a playable world map)

## 2026-10-20 display-controller checkpoint

The typed display controller now recovers native Update 06054F9E and its
retail helpers: all seven states, six fixed-point easing profiles, accelerated
springs, initial target positioning, marker arrival/visibility, camera orbit
and close-up transitions, the 16-entry trail ring, RGB-table blending, native
camera angles, directional light and depth falloff. No floating-point lerp or
replacement renderer is used. Original-disc checks verify the easing tables;
production-code fixtures exercise all states, selection changes, exact arrival,
travel-camera dependency, RGB endpoints, bounded trails and native service
calls. A separate signed-branch differential checks 400,000 easing cases.
Fixtures mock camera/matrix services; they do not prove final rendered pixels.
Native trail rendering 06055128 is also recovered: it walks the ring backward,
wrapping at 16, and submits original MCB slot 03DC through Azel's draw service.
Its fixture verifies three-point wrap, matrix push/pop balance and invalid-state
guards. Both experimental-battle and non-battle Vita configurations build.

**Important correction to the upstream dependency names:** native root Init
060540EA calls WORLD-local 0605571A (literal at 06054190), creating a 0x40-byte
child. It does not call the title `initLoadSavegameScreen` that upstream has
substituted. Display helper 060545F4 copies that child's live +8/+C/+10
position, not the current selected destination or an embedded Saturn pointer.
The new controller takes that position explicitly and refuses state 3 before
the native child is ready. The child's +3D controls marker shrink; it is named
`arrivalBusy`, not assumed to be a title save-screen flag.

Environment 06056928 is likewise an initial callback: it runs 06056658, then
installs Update 06056AD4 and Draw 06056DDE and sets ready bit 4. The latter
consumes the display's live camera/rotation and generates two native rotation
passes. Merely replacing its initial stub with a permanent Update function
would be incorrect. These environment and local-child routines are the next
recovery work, not an instruction to enable the placeholder graph.

The controller and camera/light application service compile in the Vita
experimental build, but are not wired to mode 6. The live map remains unready
for hardware testing. No new testing build is recommended at this checkpoint.

## Model setup checkpoint

`lagi_world_map_model.cpp` now ports WORLD's model initializer (06054504)
and renderer (060551AC) into typed host state. It reads the original map
hierarchy slot 01EC, static pose 05C8 and color animation 05D0 from
WORLDMAP.MCB. Visibility bit 20, translation/yaw/three-column scale order,
pose control and the four native RGB triplets are retained. No game data is
committed. `tests/world_map_model_test.ps1` checks the original-disc call
table/resource offsets and runs the actual production initializer/draw code
against mocked services. The full-resolution experimental Vita VPK builds.

This service is **not connected to mode 6 yet**. Its scale starts at zero and
visibility starts clear, exactly as native Init requires; WORLD Update owns
revealing and positioning it. Calling Draw alone is not map functionality.
Do not send this checkpoint for hardware map testing as though it were ready.

Remaining native recovery gates, in dependency order:

1. Connect the recovered display controller only after its native children
   and environment are available. 06054308's debug keyboard editor is excluded.
2. Environment initial callback 06056928, setup 06056658, Update 06056AD4 and
   Draw 06056DDE; control Init/Draw 06055740/06055924.
3. Camera task writes currently replaced by `Unimplemented()`, name task,
   destination confirmation, WORLD-local child 0605571A and native travel dispatch.
4. Owned root/overlay lifetime and shared mode-6 scene/view/composition gate.
5. Build and fixture checks, then Vita entry/selection/cancel/travel testing.

The existing black holding boundary remains deliberate until these gates
are satisfied; no stub task graph, fabricated destination screen or premature
travel path has been enabled.

## Original handoff evidence

The previous interpretation of status 0x4F as return-to-title was wrong.
Original-disc evidence:

- MAIN 06027C72..74 requests 0x4F after E014, as reconstructed.
- COMMON 00212F4A contains mode 6/subindex 0 for that status.
- COMMON dispatch slot 6 at 00212FDC points to MAIN 06027878.
- MAIN 06027880 loads the filename through 06027900 -> 0604382C: WORLD.PRG.
- Slot 5 instead points to 0602785A, which loads CHANGE.PRG. It is not WORLD.

The generated module dispatch now binds mode 6 in the experimental battle build
to a service that loads the selected native WORLD.PRG and returns a real live
child task. It does not change the story's requested status, fall through to
status zero, or reuse the module manager as its own child. This stops the
unsupported-slot -> intro loop. Other builds retain their disabled boundary.

**Display, navigation and travel are not implemented by this commit.** The
boundary intentionally remains at the outgoing black screen while the game
loop remains alive. Its one-time `[WorldMapStartup]` log explicitly says
`display/travel unported ... not a playable map`. This is not a substitute map,
temporary menu or claimed completion of world-map bring-up.

Enabling upstream `worldmap.cpp` wholesale is unsafe: Display Init/Update/Draw,
Env Update and Dragon Init/Draw are stubs; camera logic reads their absent state.
Upstream Root Init incorrectly substitutes the title save/load initializer,
which resets the same map allocator, for WORLD-local 0605571A. Its
`loadWorldOverlay` also returns its parent as the active overlay.
Those need native ports before enabling the map's task graph. The next work is
to recover WORLD 06054504/06054F9E/060551AC display state/rendering and its camera,
environment, dragon and resident-save child services; then expose mode 6 through
the shared scene bridge and verify navigation/travel on hardware.

Direct-disc tests verify the mode/entry/filename chain; the production boundary
fixture checks child ownership, data lifetime, missing-file handling and unchanged
status. Both Vita configurations compile. Neither check proves on-device map
rendering or even acceptance of this runtime handoff.

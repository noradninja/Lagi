# Native WORLD handoff boundary (not a playable world map)

## 2026-10-20 recovery checkpoint

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

1. Display Update 06054F9E and helpers 06054308, 060545DA..06054DE0:
   initial target, camera interpolation, pose/scale/visibility transitions.
2. Environment Update 06056928 and Dragon Init/Draw 06055740/06055924.
3. Camera task writes currently replaced by `Unimplemented()`, name task,
   destination confirmation, resident save child and native travel dispatch.
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
Root Init also calls the title save/load initializer, which resets the same map
allocator, and `loadWorldOverlay` returns its parent as the active overlay.
Those need native ports before enabling the map's task graph. The next work is
to recover WORLD 06054504/06054F9E/060551AC display state/rendering and its camera,
environment, dragon and resident-save child services; then expose mode 6 through
the shared scene bridge and verify navigation/travel on hardware.

Direct-disc tests verify the mode/entry/filename chain; the production boundary
fixture checks child ownership, data lifetime, missing-file handling and unchanged
status. Both Vita configurations compile. Neither check proves on-device map
rendering or even acceptance of this runtime handoff.

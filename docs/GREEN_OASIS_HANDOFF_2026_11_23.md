# Green Oasis handoff — 23 November 2026

The work Vita run cleared Lathum and reached Green Oasis (Garil field 3, subfield 2). The old build crashed after targeting the oasis monolith. The updated build packages successfully. The new Green Oasis interactions and Hopper battles still need a Vita run.

## Resume at home

The source changes were made over `35d8b8c` on `diagnostics/garil-cliff-current`. If the changes are still local to the work machine, take `Lagi-Green-Oasis-handoff.patch` home with the test VPK and matching VELF. Apply the patch only if your checkout does not already contain the Green Oasis changes.

From the home checkout in PowerShell, check for local edits before switching, then run:

```powershell
Set-Location E:\dev\Lagi
git status --short
git fetch origin
git switch diagnostics/garil-cliff-current
git pull --ff-only origin diagnostics/garil-cliff-current
git -c core.longpaths=true submodule update --init --recursive
git log -1 --oneline
```

If the branch is absent locally, use `git switch --track origin/diagnostics/garil-cliff-current`. On the `35d8b8c` baseline, run `git apply --check <patch-path>` and then `git apply <patch-path>`, replacing the placeholder with the quoted path to the downloaded patch. Use the home VitaSDK and shader compiler paths and the Release flags in [Building Lagi](BUILDING.md). A fresh build directory avoids cached feature flags. Keep the VPK and its own unstripped `lagi.velf` together.

Start the next Codex chat in the repo and point it to this file. Continue from the normal save and authored field/encounter routes.

## Source and original-disc evidence

| Area | Finding and implementation |
| --- | --- |
| Monolith | The matching dump and VELF resolve the crash to `a5SandfallEntity_Draw` calling `get3DModel` on a color stream. Both LODs now use the existing native polygon-color renderer. All 24 streams match the Disc 1 models. The cutscene camera reads its big-endian target into task-owned storage. |
| Green Oasis field | Overlay `FLD_A5` calls the shared cell setup with placement table `0608FFA8`. The adapter registers the three authored factories at `0605ED66`, `0605EDBA`, and `0605EE34`. Disc checks cover 112 placements and 237 whirlpool model frames. The small trapped-worm sprite and rescue use the original quad descriptors and story bit, with a separate segmented Baldor actor and item-box impact dust. |
| Battle scenery | Six missing `BTL_A5` environment callbacks use the shared VDP2 setup and draw. Grid variants load their authored grid and A5 resource bundle, including the 2×2 arena at `060AEDA0` for battle 14. |
| Hoppers | The Green Oasis encounter pools select battles 10 and 14. Both dispatch the existing three-enemy constructor at `060547DA`. Disc battle text slot 14 confirms the name **Hoppers**. Original encounter weights are retained. |
| Nay­nid damage | The shared target initializer returned populated hitbox values, but the Nay­nid damage adapter read the actor's uninitialized fields. The actor now receives those values and callbacks. The Queen’s homing-laser immunity remains in her target flags. The mixed swarm uses the same actor. |

The source changes live in `CMakeLists.txt` and `src/integration/lagi_oasis_sand_pits.inc`, `lagi_oasis_baldor.inc`, and `lagi_garil_battle_environment.inc`. Reconfiguration generates the corresponding Azel compilation units. Edit the source adapters for follow-up work.

## Next Vita run

1. Enter Green Oasis through the normal post-Lathum passage. Target and activate the monolith. Verify that the sandfall draws and the camera/script returns control.
2. Fly across the quicksand cells and trigger the whirlpools. Check their animation, the small worm’s appearance and rescue, the larger Baldor’s behavior, and the authored follow-up. Open an item box and check its dust sprites.
3. Encounter Hoppers naturally. Record `[BattleStartup]` with `overlay=1` and `sub=10` or `sub=14`. Verify three Hoppers, arena scenery across quadrants, attacks and damage, then results and return to Green Oasis.
4. If a Nay­nid swarm appears, check that gun shots damage both the Queen and regular Nay­nids and that the Queen still deflects homing lasers. Confirm the fight reaches results.

Pludger battles 11 and 13 have their authored scenery callbacks, but the constructor at `060585E8` is still a stub. Neither is a Hopper route. Keep the next `lagi.log`, and if there is a crash, capture the latest dump and fatal log with the matching VELF. Record the exact trigger and whether the effect was absent, clipped, or drawn incorrectly.

## Activation audit follow-up

[The activation audit](GREEN_OASIS_ACTIVATION_AUDIT.md) found that discovery
and the rendering fix do not restore the monolith activation callback/state
machine. Normal activation and saved post-activation monolith setup remain
missing. The later proximity/story-worm services are connected by generated
adapters despite stubs visible in raw upstream source. Do not infer complete
Oasis activation from the earlier package/build checkpoint.

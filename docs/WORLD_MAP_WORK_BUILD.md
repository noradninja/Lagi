# WORLD/Garil work-machine build — 2026-11-05

## Current work-machine resume — 2026-11-12

Use branch `diagnostics/garil-cliff-current`. The Gash dialogue checkpoint is
commit `608244f38485ab660f143e94efbfb17c12d0c829`; the documentation update
commit immediately after it is the branch tip to pull. The older
`feature/light-state-index` commands below describe a historical checkpoint,
not this one. `docs/CAMP_CALLBACK_AUDIT.md` records the crash, input, raw PCM
and story-data findings.

On the work machine, preserve any tracked edits before switching or pulling:

```powershell
Set-Location C:\Dev\Lagi
git status --short
if (git status --porcelain --untracked-files=no) {
    throw 'Tracked edits exist; preserve or reconcile them before pulling.'
}
git fetch origin
if ($LASTEXITCODE) { throw 'Fetch failed' }
git switch diagnostics/garil-cliff-current
if ($LASTEXITCODE) { throw 'Branch switch failed' }
git pull --ff-only origin diagnostics/garil-cliff-current
if ($LASTEXITCODE) { throw 'Pull failed; do not reset the checkout' }
git -c core.longpaths=true submodule update --init --recursive
if ($LASTEXITCODE) { throw 'Submodule update failed' }
git rev-parse HEAD
```

Use a new build directory and the current comparison settings. The successful
2026-11-11 Vita cache had Release, full resolution, native A3, WORLD, VDP2
sprite composite, RBG decoded pages/coefficient rows, stochastic filter,
dither lookup, Azel probe and ruins trim ON; GPU pass timing and battle link
audit OFF. It had both Neptune and render bridge optimization ON. Keep that
combination for this checkpoint; the older profile table below pins render
bridge optimization OFF and is not the matching build.

```powershell
$env:VITASDK = 'C:\Dev\VitaSDK'
$env:PSP2CGC = 'C:\Dev\sdk\host_tools\bin\psp2cgc.exe'
$env:PATH = "$env:VITASDK\bin;$env:PATH"
$gashBuild = 'build/gash-dialog-20261112'
cmake -S . -B $gashBuild -G Ninja `
    -DCMAKE_BUILD_TYPE=Release `
    -DLAGI_FULLRES=ON -DLAGI_BATTLE_NATIVE_BUILD=ON `
    -DLAGI_BATTLE_EXECUTE_A3=ON -DLAGI_WORLD_MAP_BRINGUP=ON `
    -DLAGI_VDP2_SPRITE_COMPOSITE=ON -DLAGI_RBG_DECODED_PAGES=ON `
    -DLAGI_RBG_COEFFICIENT_ROWS=ON -DLAGI_STOCHASTIC_FILTER=ON `
    -DLAGI_DITHER_LOOKUP=ON -DLAGI_OPTIMIZE_NEPTUNE=ON `
    -DLAGI_OPTIMIZE_RENDER_BRIDGE=ON -DLAGI_AZEL_PROBE=ON `
    -DLAGI_AZEL_RUINS_LINK_TRIM=ON -DLAGI_GPU_PASS_TIMING=OFF `
    -DLAGI_BATTLE_LINK_AUDIT=OFF
if ($LASTEXITCODE) { throw 'Configure failed' }
cmake --build $gashBuild --target Lagi.vpk --parallel 8
if ($LASTEXITCODE) { throw 'Build/package failed' }
$gashRevision = (git rev-parse --short=12 HEAD).Trim()
$gashArchive = Join-Path 'build/hardware' $gashRevision
New-Item -ItemType Directory -Force -Path $gashArchive | Out-Null
Copy-Item -LiteralPath "$gashBuild/Lagi.vpk", "$gashBuild/lagi.velf", "$gashBuild/lagi" -Destination $gashArchive
Get-FileHash -LiteralPath "$gashArchive/Lagi.vpk" -Algorithm SHA256
```

The already built checkpoint VPK on the source machine has SHA256
`CF50B47B33033C1CC0D81A50962E794A2874CAAFB5EC7399CBCD70342B829699`.
The work-machine package may differ byte for byte due to SDK or packaging
details; archive its own VPK, VELF and unstripped `lagi` together.

One Vita capture should approach Gash, hear his voice, advance dialogue with
Cross and Circle, navigate topics with both D-pad and left stick, select a
topic and exit without a crash. Continue the authored gift/recruitment route
into Garil if reachable, and retain the matching log and any dump. A full
Vita runtime pass has not yet been observed for this checkpoint. Do not set
story bits or skip dialogue to reach Gash.

Branch: `feature/light-state-index`. Read `WORLD_MAP_HANDOFF.md` for native
pipeline changes and acceptance limits. This is still bring-up, not a claim
that camp/Garil progression or the 33 ms deadline passes on hardware.

## Morning resume checkpoint - 2026-11-06

Latest projection checkpoint preserves the existing wide camera presentation.
Read `CAMP_CALLBACK_AUDIT.md` for the precise changes and hardware gates.
Recent independent commits:87840c4 native Camp NPC/segmented actor;
0c9df32 native battle gun attachment slots;1411da6 original-disc parity test.
The VDP2 projection change follows as its own atomic commit. Camp actor,
battle gun, ground stability and frame time need hardware validation; flight
gun visibility and campfire blending remain open. Do not treat source tests
or package success as those passes.

Use the profile and arguments below unchanged. For this morning choose a new
`build/world-camp-20261106` directory in place of `build/world-visual-20261105`
everywhere below. Keep the full-resolution ON setting and GPU pass timing OFF.
Use one capture through Camp interaction, Garil ground movement and return-area
bird targeting, with the matching log and retained unstripped symbols.

## Pull without losing local work

Run in PowerShell. If the tracked-edit guard stops, preserve/reconcile those
edits first; do not reset or clean the checkout. Untracked assets are left alone.

```powershell
Set-Location C:\Dev\Lagi
git status --short
if (git status --porcelain --untracked-files=no) {
    throw 'Local tracked edits exist; preserve/reconcile them before pulling.'
}
git fetch origin
if ($LASTEXITCODE) { throw 'Fetch failed' }
git switch feature/light-state-index
if ($LASTEXITCODE) { throw 'Branch switch failed' }
git pull --ff-only origin feature/light-state-index
if ($LASTEXITCODE) { throw 'Pull failed; do not force/reset' }
git -c core.longpaths=true submodule update --init --recursive
if ($LASTEXITCODE) { throw 'Submodule update failed' }
git rev-parse HEAD
```

## Configure the exact renderer profile

Use a NEW dedicated build directory, not the older `build` cache. The profile
forces the important WORLD/native-composition/cache switches even when reused;
the extra arguments below also pin the other settings from the tested cache.
These paths match this work machine's installed SDK and shader compiler.
Use build/world-visual-20261105 as a fresh directory for this checkpoint.
If it already contains an older configuration, choose a new directory name.

```powershell
$env:VITASDK = 'C:\Dev\VitaSDK'
$env:PSP2CGC = 'C:\Dev\sdk\host_tools\bin\psp2cgc.exe'
$env:PATH = "$env:VITASDK\bin;$env:PATH"
$worldBuild = 'build/world-visual-20261105'
$worldConfigure = @(
    '-S', '.', '-B', $worldBuild, '-G', 'Ninja',
    '-C', 'cmake/world_garil_profile.cmake',
    '-DLAGI_OPTIMIZE_NEPTUNE:BOOL=ON',
    '-DLAGI_OPTIMIZE_RENDER_BRIDGE:BOOL=OFF',
    '-DLAGI_STOCHASTIC_FILTER:BOOL=ON',
    '-DLAGI_DITHER_LOOKUP:BOOL=ON',
    '-DLAGI_BATTLE_LINK_AUDIT:BOOL=OFF',
    '-DLAGI_AZEL_PROBE:BOOL=ON',
    '-DLAGI_AZEL_RUINS_LINK_TRIM:BOOL=ON'
)
cmake @worldConfigure
if ($LASTEXITCODE) { throw 'Configure failed' }
cmake --build $worldBuild --parallel 8
if ($LASTEXITCODE) { throw 'Build/package failed' }
```

Required cache values (check `build/world-visual-20261105/CMakeCache.txt`):

| Setting | Value |
|---|---|
| `CMAKE_BUILD_TYPE` | `Release` |
| `LAGI_FULLRES` | `ON` |
| `LAGI_BATTLE_NATIVE_BUILD`, `LAGI_BATTLE_EXECUTE_A3`, `LAGI_WORLD_MAP_BRINGUP` | `ON` |
| `LAGI_VDP2_SPRITE_COMPOSITE` | `ON` |
| `LAGI_RBG_DECODED_PAGES`, `LAGI_RBG_COEFFICIENT_ROWS` | `ON` |
| `LAGI_OPTIMIZE_NEPTUNE` | `ON` |
| `LAGI_OPTIMIZE_RENDER_BRIDGE` | `OFF` |
| `LAGI_GPU_PASS_TIMING`, `LAGI_FIELD_MOTION_TRACE`, `LAGI_BATTLE_LINK_AUDIT` | `OFF` |
| `LAGI_STOCHASTIC_FILTER`, `LAGI_DITHER_LOOKUP` | `ON` |
| `LAGI_AZEL_PROBE`, `LAGI_AZEL_RUINS_LINK_TRIM` | `ON` |

Important: current accepted-profile comparison uses `LAGI_FULLRES=ON` (960x544),
not the historical 480x272 town setup. Do not silently change resolution while
comparing performance. Sprite composition is required for accepted water/cloud
layer behavior; disabling it is not an equivalent baseline. Both RBG fast-path
switches must remain enabled. Serialized GPU-pass timing adds diagnostic waits;
do NOT enable it for normal performance captures. Game background sampling stays
point-filtered; these commands do not introduce bilinear game backgrounds.

## Preserve the package AND symbols before deployment

Deploy the VPK from this build directory, not `build/Lagi.vpk`. Save the exact
unstripped `lagi` executable as well as `lagi.velf`; later builds overwrite both.
Do not associate an old dump with the newest file just because its name matches.

```powershell
$worldRevision = (git rev-parse --short=12 HEAD).Trim()
$worldArchive = Join-Path 'build/hardware' $worldRevision
New-Item -ItemType Directory -Force -Path $worldArchive | Out-Null
Copy-Item -LiteralPath "$worldBuild/Lagi.vpk", "$worldBuild/lagi.velf", "$worldBuild/lagi" -Destination $worldArchive
Get-FileHash -LiteralPath "$worldArchive/Lagi.vpk" -Algorithm SHA256 | Format-List Algorithm,Hash,Path
git rev-parse HEAD
```

One coherent capture can cover WORLD destination models, selection/confirmation,
Garil entry/control and return/re-entry where reachable, with the matching log.
If camp crashes, save the dump and use this archived revision's symbols.

## Existing camp dump limitation

The supplied `psp2core-1793844053-0x0019d82f9b-eboot.bin.psp2dmp` was received.
It parses and records main thread `LAGI00001`, PC `810E4F4E`, data-fault address
`00000025`, stop reason `00030004`. It does NOT match the supplied later
`build/world-garil/lagi.velf`: that path now holds runtime093f257. Dump module
executable segment size is `18853C`, versus that ELF's `189280`. The old ELF is
unavailable, as confirmed by the user. Symbol names, disassembly and EHABI unwind
computed with this later ELF are not trustworthy crash attribution. No camp
crash fix is claimed. A reproduction using a preserved matching build is the
next reliable diagnostic checkpoint; do not guess a fix from nearby symbols.

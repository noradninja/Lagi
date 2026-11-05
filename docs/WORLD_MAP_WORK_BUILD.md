# WORLD/Garil work-machine build — 2026-11-05

Branch: `feature/light-state-index`. Read `WORLD_MAP_HANDOFF.md` for native
pipeline changes and acceptance limits. This is still bring-up, not a claim
that camp/Garil progression or the 33 ms deadline passes on hardware.

## Pull without losing local work

Run in PowerShell. If the tracked-edit guard stops, preserve/reconcile those
edits first; do not reset or clean the checkout. Untracked assets are left alone.

```powershell
Set-Location E:\dev\Lagi
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
git submodule update --init --recursive
if ($LASTEXITCODE) { throw 'Submodule update failed' }
git rev-parse HEAD
```

## Configure the exact renderer profile

Use a NEW dedicated build directory, not the older `build` cache. The profile
forces the important WORLD/native-composition/cache switches even when reused;
the extra arguments below also pin the other settings from the tested cache.
SDK paths may be adjusted to the work machine's installations only.

```powershell
$env:VITASDK = 'E:\dev\VitaSDK'
$env:PSP2CGC = 'E:\PSVITA\sdk\host_tools\bin\psp2cgc.exe'
$worldBuild = 'build/world-garil-work'
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

Required cache values (check `build/world-garil-work/CMakeCache.txt`):

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

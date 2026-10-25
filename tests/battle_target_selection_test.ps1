param([string]$BuildDirectory='build-battle-dev')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-target-selection-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
$source=Get-Content -Raw "$repo/$BuildDirectory/azel_upstream/battle_battleOverlay_C.cpp"
$start=$source.IndexOf('bool sBattleOverlayTask_C_IsTargetableForGun(')
$end=$source.IndexOf('void sBattleOverlayTask_C_Draw(',$start)
if($start -lt 0 -or $end -le $start){throw 'Shared target-selection production boundaries changed'}
[IO.File]::WriteAllText((Join-Path $scratch 'battle_target_selection_source.inc'),$source.Substring($start,$end-$start))
& cl /nologo /O2 /std:c++20 /EHsc "/I$scratch" "$PSScriptRoot/battle_target_selection_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Target selection fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Target selection fixture failed'}

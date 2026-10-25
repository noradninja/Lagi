param([string]$Build='build-battle-dev',[string]$ScratchRoot='../battle-stat-count-test')
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $root $ScratchRoot))
New-Item -ItemType Directory -Force $scratch | Out-Null
$generated=Get-Content -Raw "$root/$Build/azel_upstream/battle_battleEngine.cpp"
if(-not $generated.Contains('#include "lagi_battle_stat_count.inc"')){throw 'Production statistic adapter not active'}
Copy-Item -LiteralPath "$root/src/integration/lagi_battle_stat_count.inc" -Destination "$scratch/stat_count_body.inc"
& cl /nologo /O2 /std:c++20 /EHsc "/I$scratch" "$PSScriptRoot/battle_stat_count_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE-ne 0){throw 'Battle statistic fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE-ne 0){throw 'Battle statistic fixture failed'}

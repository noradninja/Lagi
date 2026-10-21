param([string]$ScratchRoot='../battle-urchin-ambient-test',[string]$Build='build-battle-dev')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $repo $ScratchRoot))
New-Item -ItemType Directory -Force $scratch | Out-Null
& cl /nologo /O2 /std:c++20 /EHsc "/I$repo/src/integration" "$PSScriptRoot/battle_urchin_ambient_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Native ambient emitter test compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Native ambient emitter test failed'}
$dispatch=Get-Content -Raw "$repo/$Build/azel_upstream/battle_BTL_A3_BTL_A3_data.cpp"
if($dispatch -notmatch 'case 0x06056AF2:\s*return lagiCreateUrchinAmbientEffect\(pParent\);'){throw 'Recovered effect factory missing from live A3 dispatcher'}
$urchin=Get-Content -Raw "$repo/$Build/azel_upstream/battle_BTL_A3_urchin.cpp"
if(!$urchin.Contains('#include "lagi_battle_urchin_ambient.inc"')){throw 'Native emitter implementation not compiled with typed urchin position'}

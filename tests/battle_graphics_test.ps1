param([string]$ScratchRoot='../battle-graphics-test')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $repo $ScratchRoot))
New-Item -ItemType Directory -Force $scratch | Out-Null
$source=Get-Content -Raw "$repo/src/integration/lagi_battle_graphics_vita.cpp"
$begin=$source.IndexOf('extern sVdpVar1* vdpVar2;')
if($begin -lt 0){throw 'Production graphics service boundary changed'}
[IO.File]::WriteAllText("$scratch/service.inc",$source.Substring($begin))
& cl /nologo /O2 /std:c++20 /EHsc "/I$scratch" "$PSScriptRoot/battle_graphics_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Graphics service test compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Graphics service test failed'}

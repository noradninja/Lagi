param([string]$ScratchRoot='../battle-recoil-test',[string]$Build='build-battle-dev')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $repo $ScratchRoot))
New-Item -ItemType Directory -Force $scratch | Out-Null
$source=Get-Content -Raw "$repo/$Build/azel_upstream/battle_BTL_A3_urchin.cpp"
$statements=[regex]::Matches($source,'pThis->(?:m2C_positionDelta|m50_rotationDelta)\[[012]\] \+= MTH_Mul\(pThis->mCC->m3[04]_hit(?:Translation|Rotation),[^;]+;')
if($statements.Count -ne 6){throw 'Expected six actual native hit impulse statements'}
[IO.File]::WriteAllText("$scratch/recoil.inc",($statements | ForEach-Object {$_.Value}) -join "`n")
& cl /nologo /O2 /EHsc /std:c++20 "/I$scratch" "$PSScriptRoot/battle_recoil_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Hit recoil fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Hit recoil fixture failed'}

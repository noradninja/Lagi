param([string]$ScratchRoot='../field-water-wildlife-test',[string]$BattleBuild='build-battle-dev')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $repo $ScratchRoot))
New-Item -ItemType Directory -Force $scratch | Out-Null
$native=Get-Content -Raw "$repo/$BattleBuild/azel_upstream/field_field_a3_o_fld_a3_2.cpp"
$begin=$native.IndexOf('            // FLD_A3 06055BF0..06055C60:')
if($begin -lt 0){throw 'Wildlife native orbit adapter missing'}
$end=$native.IndexOf('            updateLCSTarget(&pThis->m40_LCSTarget);',$begin)
if($end -le $begin){throw 'Wildlife target update missing'}
$end+='            updateLCSTarget(&pThis->m40_LCSTarget);'.Length
[IO.File]::WriteAllText("$scratch/motion.inc",'void motion(Task* pThis){'+$native.Substring($begin,$end-$begin)+'}')
& cl /nologo /O2 /std:c++20 /EHsc "/I$scratch" "$PSScriptRoot/field_water_wildlife_motion_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Wildlife motion test compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Wildlife motion test failed'}

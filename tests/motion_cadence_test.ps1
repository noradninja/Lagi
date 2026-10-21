$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$cmake=Get-Content -Raw "$repo/CMakeLists.txt"
if(!$cmake.Contains('option(LAGI_FIELD_MOTION_TRACE "Bounded field motion/snapshot cadence diagnostics" OFF)')){throw 'Motion trace must default OFF'}
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-motion-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
& cl /nologo /O2 /std:c++20 /EHsc "$PSScriptRoot/motion_cadence_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Motion cadence fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Motion cadence fixture failed'}

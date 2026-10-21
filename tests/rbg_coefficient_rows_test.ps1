$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-rbg-rows-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
$source=Get-Content -Raw "$repo/extern/Azel/AzelLib/renderer_vdp2.cpp"
$start=$source.IndexOf('static s32 readCoefficientFromVRAM(')
$end=$source.IndexOf('static void renderLayerCPU_RBG0(', $start)
if($start -lt 0 -or $end -lt $start){throw 'Native coefficient oracle anchors changed'}
[IO.File]::WriteAllText((Join-Path $scratch 'native_coefficient_reference.inc'),$source.Substring($start,$end-$start))
& cl /nologo /O2 /std:c++20 /EHsc "$PSScriptRoot/rbg_coefficient_rows_test.cpp" "/I$scratch" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Coefficient row fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Coefficient row fixture failed'}

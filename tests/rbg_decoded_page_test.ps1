$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-rbg-page-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
$native=Get-Content -Raw "$repo/extern/Azel/AzelLib/renderer_vdp2.cpp"
$start=$native.IndexOf('static u32 sampleTileAtCoordinate(')
$end=$native.IndexOf('// Fixed-point multiply matching Saturn 16.16:', $start)
if($start -lt 0 -or $end -lt $start){throw 'Native tile reference anchors changed'}
# Generated reference artifact, not an edit to upstream or a committed copy.
[IO.File]::WriteAllText((Join-Path $scratch 'native_rbg_reference.inc'),$native.Substring($start,$end-$start))
& cl /nologo /O2 /std:c++20 /EHsc "$PSScriptRoot/rbg_decoded_page_test.cpp" "/I$scratch" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Native page fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Native page fixture failed'}

$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-frontend-rbg-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
$source=Get-Content -Raw "$repo/src/platform/vita/neptune_renderer.cpp"
$start=$source.IndexOf('static bool drawVdp2Rbg0Gpu(')
$end=$source.IndexOf('#include "neptune_rbg_decoded_pages.inc"', $start)
if($start -lt 0 -or $end -lt $start){throw 'Production RBG draw anchors changed'}
[IO.File]::WriteAllText((Join-Path $scratch 'frontend_rbg_draw.inc'),$source.Substring($start,$end-$start))
& cl /nologo /O2 /std:c++20 /EHsc "$PSScriptRoot/frontend_rbg_draw_test.cpp" "/I$scratch" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Frontend RBG draw fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Frontend RBG draw fixture failed'}

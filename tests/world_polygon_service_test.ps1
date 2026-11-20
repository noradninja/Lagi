$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-world-polygon-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
$source=(Get-Content -Raw "$repo/src/integration/lagi_world_polygon_vita.cpp").Replace('#include "lagi/lagi_azel_upstream_prelude.h"','')
[IO.File]::WriteAllText((Join-Path $scratch 'world_polygon_service.inc'),$source)
& cl /nologo /O2 /std:c++20 /EHsc "/I$repo/include" "/I$scratch" "$PSScriptRoot/world_polygon_service_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'World polygon service compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'World polygon service fixture failed'}

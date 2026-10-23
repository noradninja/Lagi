$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-topology-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
& cl /nologo /O2 /std:c++20 /EHsc "/I$repo/include" "/I$repo/extern/Azel/AzelLib" "$PSScriptRoot/live_model_topology_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Topology fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Topology fixture failed'}

$ErrorActionPreference='Stop'
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-vdp2-sprite-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
& cl /nologo /std:c++20 /EHsc "$PSScriptRoot/vdp2_sprite_color_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'VDP2 sprite fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'VDP2 sprite fixture failed'}

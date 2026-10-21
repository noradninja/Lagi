$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-name-cursor-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
$source=(Get-Content -Raw "$repo/src/integration/lagi_name_cursor.inc") -replace '(?m)^#include[^\r\n]*',''
[IO.File]::WriteAllText((Join-Path $scratch 'name_cursor_fixture.inc'),$source)
& cl /nologo /O2 /std:c++20 /EHsc "$PSScriptRoot/name_cursor_test.cpp" "/I$scratch" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Cursor fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Cursor fixture failed'}

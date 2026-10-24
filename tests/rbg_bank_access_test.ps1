$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-rbg-banks-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
& cl /nologo /O2 /std:c++20 /EHsc "/I$repo/include" "$PSScriptRoot/rbg_bank_access_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'RBG bank test compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'RBG bank test failed'}

$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-rbg-work-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
$baseline=(& git -C $repo show fc94097:include/lagi/rbg_decoded_page.h) -join "`n"
if($LASTEXITCODE -ne 0){throw 'Unable to read accepted decoder baseline'}
[IO.File]::WriteAllText("$scratch/baseline.inc",$baseline.Replace('namespace lagi {','namespace baseline {'))
& cl /nologo /O2 /std:c++20 /EHsc "/I$scratch" "$PSScriptRoot/rbg_decode_work_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Cold decoder fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Cold decoder equivalence failed'}

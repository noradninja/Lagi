param([string]$CuePath,[string]$BinPath)
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-cdda-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
& cl /nologo /std:c++20 /EHsc "/I$repo/include" "$PSScriptRoot/cdda_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'CDDA fixture compile failed'}
if($CuePath -and $BinPath){& "$scratch/test.exe" "$scratch/audio.bin" $CuePath $BinPath}
else {& "$scratch/test.exe" "$scratch/audio.bin"}
if($LASTEXITCODE -ne 0){throw 'CDDA fixture failed'}

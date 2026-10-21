$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-play-time-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
$source=Get-Content -Raw "$repo/extern/Azel/AzelLib/kernel/moduleManager.cpp"
$begin=$source.IndexOf('void moduleManager_Update(s_moduleManager*)')
$end=$source.IndexOf('s32 exitMenuTaskSub1TaskDrawSub1(', $begin)
if($begin -lt 0 -or $end -lt $begin){throw 'Native play-time consumer anchors changed'}
[IO.File]::WriteAllText((Join-Path $scratch 'play_time_native_consumer.inc'),$source.Substring($begin,$end-$begin))
& cl /nologo /O2 /std:c++20 /EHsc "$PSScriptRoot/play_time_test.cpp" "/I$scratch" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Play-time fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Play-time fixture failed'}

param([string]$ScratchRoot='../frontend-font-io-test')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $repo $ScratchRoot))
New-Item -ItemType Directory -Force $scratch | Out-Null
$source=Get-Content -Raw "$repo/src/platform/vita/iso9660.cpp"
$begin=$source.IndexOf('bool read_file_from_disc(');$end=$source.IndexOf('bool init()',$begin)
if($begin -lt 0 -or $end -le $begin){throw 'Scoped disc reader boundary changed'}
[IO.File]::WriteAllText("$scratch/scoped_disc.inc",$source.Substring($begin,$end-$begin))
& cl /nologo /std:c++20 /EHsc "/I$repo/include" "/I$scratch" "$PSScriptRoot/frontend_font_disc_test.cpp" "/Fe:$scratch/disc-test.exe" "/Fo:$scratch/disc-test.obj"
if($LASTEXITCODE -ne 0){throw 'Scoped disc reader compile failed'}
& "$scratch/disc-test.exe"
if($LASTEXITCODE -ne 0){throw 'Scoped disc reader failed'}
& cl /nologo /std:c++20 /EHsc "/I$repo/include" "$PSScriptRoot/frontend_font_loader_test.cpp" "$repo/src/frontend_font.cpp" "$repo/src/platform/vita/frontend_font_vita.cpp" "/Fe:$scratch/loader-test.exe" "/Fo:$scratch/"
if($LASTEXITCODE -ne 0){throw 'Frontend font loader compile failed'}
foreach($mode in @('loose','disc','missing','invalid')) {
 $folder=Join-Path $scratch $mode
 New-Item -ItemType Directory -Force $folder | Out-Null
 & "$scratch/loader-test.exe" $mode $folder
 if($LASTEXITCODE -ne 0){throw "Frontend loader failed: $mode"}
}

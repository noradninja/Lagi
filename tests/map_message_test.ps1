param([string]$ScratchRoot='..\map-message-test',[string]$DiscPath='D:/Users/Noradninja/Desktop/Panzer Dragoon Saga Disc 1 of 4 (U)(Saturn)/042 Panzer Dragoon Saga Disc 1 of 4 (U).bin')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $repo $ScratchRoot))
New-Item -ItemType Directory -Force -Path $scratch | Out-Null
$source=Get-Content -Raw (Join-Path $repo 'src/integration/lagi_map_message_vita.cpp')
$begin=$source.IndexOf('namespace {')
if($begin -lt 0){throw 'Native Map message production boundary missing'}
Set-Content -LiteralPath (Join-Path $scratch 'map_message_production.inc') -Value $source.Substring($begin)
$parent=Get-Content -Raw (Join-Path $repo 'src/integration/lagi_map_menu_vita.cpp')
if(!$parent.Contains('lagiAzelPresentMapMessage(0,') -or $parent.Contains('drawObjectName("FIELD MAP")') -or
 $parent.Contains('drawObjectName(self->mapOwned')){throw 'Temporary Map prompt or inventory-based message remains'}
$exe=Join-Path $scratch 'map_message_test.exe'
& cl /nologo /std:c++20 /EHsc "/I$repo/include" "/I$scratch" (Join-Path $PSScriptRoot 'map_message_test.cpp') "/Fe:$exe" "/Fo:$scratch/"
if($LASTEXITCODE -ne 0){throw 'Map message host compile failed'}
if($DiscPath){& $exe $DiscPath}else{& $exe}
if($LASTEXITCODE -ne 0){throw 'Map message host execution failed'}

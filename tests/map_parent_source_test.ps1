param([string]$ScratchRoot='../map-parent-source-test')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $repo $ScratchRoot))
New-Item -ItemType Directory -Force $scratch | Out-Null
foreach($part in @(
 @('src/integration/lagi_map_parent_source_vita.cpp','bool lagiAzelResolveMapParentSource(','source.inc'),
 @('src/integration/lagi_map_menu_vita.cpp','p_workArea createMapTask(','dispatch.inc'))) {
 $source=Get-Content -Raw "$repo/$($part[0])"
 $start=$source.IndexOf($part[1]);if($start -lt 0){throw 'Map source/dispatch boundary missing'}
 [IO.File]::WriteAllText("$scratch/$($part[2])",$source.Substring($start))
}
& cl /nologo /std:c++20 /EHsc "/I$repo/include" "/I$scratch" "$PSScriptRoot/map_parent_source_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Map source/dispatch host compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Map source/dispatch assertions failed'}

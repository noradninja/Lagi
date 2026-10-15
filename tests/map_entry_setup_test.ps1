param([string]$ScratchRoot='..\map-entry-test')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $repo $ScratchRoot))
New-Item -ItemType Directory -Force -Path $scratch | Out-Null
$source=Get-Content -Raw (Join-Path $repo 'src/integration/lagi_azel_map_menu.cpp')
$ownershipBegin=$source.IndexOf('bool lagiAzelCurrentFieldMapOwned(')
$ownershipEnd=$source.IndexOf('static unsigned drawMapObjectMarkersSlot(', $ownershipBegin)
if($ownershipBegin -lt 0 -or $ownershipEnd -le $ownershipBegin){throw 'Map ownership binding missing'}
Set-Content -LiteralPath (Join-Path $scratch 'map_ownership_production.inc') -Value $source.Substring($ownershipBegin,$ownershipEnd-$ownershipBegin)
$markerEnd=$source.IndexOf('void lagiAzelRestoreMapViewPriorities(', $ownershipEnd)
if($markerEnd -le $ownershipEnd){throw 'Map marker function missing'}
Set-Content -LiteralPath (Join-Path $scratch 'map_markers_production.inc') -Value $source.Substring($ownershipEnd,$markerEnd-$ownershipEnd)
$deleteBegin=$source.IndexOf('void lagiAzelRestoreMapViewPriorities(')
$deleteEnd=$source.IndexOf('bool lagiAzelSetupMapEntry(', $deleteBegin)
if($deleteBegin -lt 0 -or $deleteEnd -le $deleteBegin){throw 'Map child cleanup missing'}
Set-Content -LiteralPath (Join-Path $scratch 'map_delete_production.inc') -Value $source.Substring($deleteBegin,$deleteEnd-$deleteBegin)
$begin=$source.IndexOf('bool lagiAzelSetupMapEntry(')
$end=$source.IndexOf('bool lagiAzelLoadFieldMapResources(', $begin)
if($begin -lt 0 -or $end -le $begin){throw 'Map entry function missing'}
Set-Content -LiteralPath (Join-Path $scratch 'map_entry_production.inc') -Value $source.Substring($begin,$end-$begin)
$exe=Join-Path $scratch 'map_entry_test.exe'
& cl /nologo /std:c++20 /EHsc "/I$scratch" "/I$repo/include" (Join-Path $PSScriptRoot 'map_entry_setup_test.cpp') "/Fe:$exe" "/Fo:$scratch/"
if($LASTEXITCODE -ne 0){throw 'Map entry compile failed'}
& $exe
if($LASTEXITCODE -ne 0){throw 'Map entry execution failed'}

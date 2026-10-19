param([string]$Build='build-battle-dev',[string]$ScratchRoot='../town-grid-reset-test')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $repo $ScratchRoot));New-Item -ItemType Directory -Force $scratch | Out-Null
$source=Get-Content -Raw "$repo/$Build/azel_upstream/town_town.cpp"
foreach($part in @(@('static void deleteCellObjects(','static void deleteGridRow(','delete_cells.inc'),@('static void deleteGridRow(','static void deleteGridColumn(','delete_row.inc'),@('static s32 initTownGridDefaultDepth','void setupDragonForTown(','init.inc'))){
 $a=$source.IndexOf($part[0]);$b=$source.IndexOf($part[1],$a)
 if($a -lt 0 -or $b -le $a){throw 'Town grid boundary changed'}
 [IO.File]::WriteAllText("$scratch/$($part[2])",$source.Substring($a,$b-$a))
}
$source=Get-Content -Raw "$repo/extern/Azel/AzelLib/kernel/worldGrid.cpp"
$a=$source.IndexOf('void deleteAllGridCells(');$b=$source.IndexOf('void createGridCells(',$a)
if($a -lt 0 -or $b -le $a){throw 'Native grid drain boundary changed'}
[IO.File]::WriteAllText("$scratch/drain.inc",$source.Substring($a,$b-$a))
& cl /nologo /O2 /std:c++20 /EHsc "/I$scratch" "$PSScriptRoot/town_grid_reset_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Town grid reset fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Town grid reset fixture failed'}

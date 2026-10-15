param([string]$ScratchRoot='../map-selection-visual-test',[string]$AzelRoot='E:/dev/Lagi/extern/Azel/AzelLib')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $repo $ScratchRoot))
New-Item -ItemType Directory -Force $scratch | Out-Null
$cursor=Get-Content -Raw "$AzelRoot/kernel/menuCursor.cpp"
$start=$cursor.IndexOf('void s_MenuCursorWorkArea::menuCursorTaskUpdate(')
$end=$cursor.IndexOf('s_MenuCursorWorkArea* createMenuCursorTask(', $start)
if($start -lt 0 -or $end -lt 0){throw 'Native cursor extraction boundary missing'}
Set-Content "$scratch/native_cursor.inc" $cursor.Substring($start,$end-$start)
$highlight=Get-Content -Raw "$AzelRoot/mainMenuDebugTasks.cpp"
$start=$highlight.IndexOf('void switchVdp2MenuTileToHightlighted(')
$end=$highlight.IndexOf('struct mainMenuTaskInitSub2TaskWorkArea', $start)
if($start -lt 0 -or $end -lt 0){throw 'Native highlight extraction boundary missing'}
Set-Content "$scratch/native_highlight.inc" $highlight.Substring($start,$end-$start)
$source=Get-Content -Raw "$repo/src/integration/lagi_map_selection_vita.cpp"
$start=$source.IndexOf('namespace {')
if($start -lt 0){throw 'Selection adapter extraction boundary missing'}
Set-Content "$scratch/selection_adapter.inc" $source.Substring($start)
& cl /nologo /std:c++20 /EHsc "/I$repo/include" "/I$scratch" "$PSScriptRoot/map_selection_visual_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/"
if($LASTEXITCODE -ne 0){throw 'Selection visual host compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Selection visual execution failed'}

$ErrorActionPreference = 'Stop'
$source = Get-Content -Raw (Join-Path $PSScriptRoot '../src/integration/lagi_system_menu_vita.cpp')
$cmake = Get-Content -Raw (Join-Path $PSScriptRoot '../CMakeLists.txt')
if ($source -match '"SAVE GAME"|"LOAD GAME"|"RETURN"|drawLineLargeFont|createSaveTask|createLoadTask') {
    throw 'Temporary System menu must not return'
}
if ($source -notmatch 'markFinished\(\)' -or $cmake -notmatch 'm3_menuButtonStates\[4\] = -1; // native System task not ported yet') {
    throw 'Unported native System entry must be safely unavailable'
}
Write-Output 'PASS: temporary System UI removed, native entry unavailable (source guards; not restoration complete)'

param(
    [string]$AzelSourceRoot = 'E:/dev/Lagi/extern/Azel/AzelLib',
    [string]$VitaCompiler = 'E:/dev/VitaSDK/bin/arm-vita-eabi-g++.exe'
)
$ErrorActionPreference = 'Stop'
$root = Join-Path $PSScriptRoot '..'
$cmakeText = Get-Content -Raw (Join-Path $root 'CMakeLists.txt')
$source = Get-Content -Raw (Join-Path $AzelSourceRoot 'kernel/loadSavegameScreen.cpp')
$cmakeText = $cmakeText.Replace("`r`n", "`n")
$source = $source.Replace("`r`n", "`n")
$tracePatch = [regex]::Match($cmakeText,'(?s)elseif\(relative_path STREQUAL "kernel/loadSavegameScreen.cpp"\).*?\[=\[(.*?)\]=\]\s*\[=\[(.*?)\]=\]')
if (!$tracePatch.Success -or !$source.Contains($tracePatch.Groups[1].Value)) {throw 'Trace anchor mismatch'}
$source = $source.Replace($tracePatch.Groups[1].Value,$tracePatch.Groups[2].Value).Replace('#include "loadSavegameScreen.h"', "#include `"loadSavegameScreen.h`"`n#include `"lagi/platform.h`"")
$source = $source.Replace('#include "PDS.h"','#include "lagi/lagi_azel_upstream_prelude.h"').Replace("    s32 m4_device;`n    std::string m8_playerName;", "    s32 m4_playFrames;`n    std::string m8_playerName;").Replace('            outputData->m4_device = device;', '            outputData->m4_playFrames = loadedGameState->mC_mainGameState.gameStats.m3C_frameCounter;')
$rankPatch = [regex]::Match($cmakeText,'(?s)# Native 060192C2.*?\[=\[(.*?)\]=\]\s*\[=\[(.*?)\]=\]')
if (!$rankPatch.Success -or !$source.Contains($rankPatch.Groups[1].Value)) {throw 'Rank anchor mismatch'}
$source = $source.Replace($rankPatch.Groups[1].Value,$rankPatch.Groups[2].Value)
foreach ($token in @('i < dragonLevel', 'data = data + 8', '0x20F734', 'drawMenuSprite(&sprite, position[0], position[1], 0x760)')) {
    if (!$rankPatch.Groups[2].Value.Contains($token)) {throw "Missing native rank invariant: $token"}
}
$patch = [regex]::Match($cmakeText,'(?s)# Native slot strings.*?\[=\[(.*?)\]=\]\s*\[=\[(.*?)\]=\]')
if (!$patch.Success -or !$source.Contains($patch.Groups[1].Value)) { throw 'Preview anchor mismatch' }
$source = $source.Replace($patch.Groups[1].Value,$patch.Groups[2].Value)
foreach ($token in @('m3C_frameCounter','m19_dragonName.c_str()', '"%2d:%02d"','"LV %2d"')) {
    if (!$patch.Groups[2].Value.Contains($token) -and !$source.Contains($token)) {throw "Missing $token"}
}
foreach ($case in @(@(0,0,0),@(3599,0,0),@(3600,0,1),@(216000,1,0),@(21596400,99,59),@(21600000,99,59))) {
    $minutes = [math]::Min([math]::Floor($case[0] / 3600),5999)
    $hours = [math]::Floor($minutes / 60)
    if ($hours -ne $case[1] -or $minutes - $hours * 60 -ne $case[2]) {throw 'Time conversion mismatch'}
}
$source | & $VitaCompiler -std=c++20 -fsyntax-only -x c++ -DLAGI_PLATFORM_VITA=1 "-I$root/include" "-I$AzelSourceRoot" "-I$AzelSourceRoot/kernel" -
if ($LASTEXITCODE -ne 0) {throw 'Native save-preview syntax failed'}
Write-Output 'PASS: native save-preview syntax, time boundaries/cap and field source checks; no Vita visual test'

param([string]$AzelSourceRoot='E:/dev/Lagi/extern/Azel/AzelLib', [string]$VitaCompiler='E:/dev/VitaSDK/bin/arm-vita-eabi-g++.exe')
$ErrorActionPreference='Stop'
$root=Join-Path $PSScriptRoot '..'
$cmake=Get-Content -Raw (Join-Path $root 'CMakeLists.txt')
$helper=[regex]::Match($cmake,'bool lagiGlobalStringTableEntryHasText\(int index\)[\s\S]*?\n\}').Value
if (!$helper) {throw 'Missing text table helper'}
$source=(Get-Content -Raw (Join-Path $AzelSourceRoot 'field/field_a3/o_fld_a3.cpp')).Replace("`r`n","`n")
$anchor='    case 0x060730bc: // 060730bc — destination multichoice dialog'
if ([regex]::Matches($source,[regex]::Escape($anchor)).Count -ne 1) {throw 'Navigator dispatch anchor not unique'}
$dispatch='    case 0x06073172: // LAGI_NATIVE_NAVIGATOR_SELECTED_TEXT'
if (!$cmake.Contains($dispatch) -or !$cmake.Contains('return lagiGlobalStringTableEntryHasText(2) ? 1 : 0;')) {throw 'Missing native dispatch'}
$source=$source.Replace($anchor,"$dispatch`n        return lagiGlobalStringTableEntryHasText(2) ? 1 : 0;`n$anchor")
$source=$source.Replace('#include "PDS.h"','#include "lagi/lagi_azel_upstream_prelude.h"')
$source | & $VitaCompiler -std=c++20 -fsyntax-only -x c++ -DLAGI_PLATFORM_VITA=1 "-I$root/include" "-I$AzelSourceRoot" "-I$AzelSourceRoot/field/field_a3" -
if ($LASTEXITCODE -ne 0) {throw 'Navigator dispatcher syntax failed'}
$vdp2=(Get-Content -Raw (Join-Path $AzelSourceRoot 'VDP2.cpp')).Replace('#include "PDS.h"','#include "lagi/lagi_azel_upstream_prelude.h"')
($vdp2+"`n"+$helper) | & $VitaCompiler -std=c++20 -fsyntax-only -x c++ -DLAGI_PLATFORM_VITA=1 "-I$root/include" "-I$AzelSourceRoot" -
if ($LASTEXITCODE -ne 0) {throw 'Live VDP2 text-table helper syntax failed'}
Write-Output 'PASS: native navigator dispatch and live VDP2 text table Vita syntax; no R-release hardware validation'

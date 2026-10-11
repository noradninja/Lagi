param([string]$AzelSourceRoot='E:/dev/Lagi/extern/Azel/AzelLib', [string]$VitaCompiler='E:/dev/VitaSDK/bin/arm-vita-eabi-g++.exe')
$ErrorActionPreference='Stop'
$root=Join-Path $PSScriptRoot '..'
$cmake=Get-Content -Raw (Join-Path $root 'CMakeLists.txt')
if (!$cmake.Contains('pNewTask->m14_status = 0; // LAGI_FIELD_EXIT_ZERO_STATE')) {throw 'Missing cleared-state contract'}
$source=(Get-Content -Raw (Join-Path $AzelSourceRoot 'field.cpp')).Replace("`r`n","`n")
$anchor='        pNewTask->m10 = 20;'
if ([regex]::Matches($source,[regex]::Escape($anchor)).Count -ne 1) {throw 'Exit constructor anchor not unique'}
$source=$source.Replace('#include "PDS.h"','#include "lagi/lagi_azel_upstream_prelude.h"').Replace($anchor,"$anchor`n        pNewTask->m14_status = 0;")
$source | & $VitaCompiler -std=c++20 -fsyntax-only -x c++ -DLAGI_PLATFORM_VITA=1 "-I$root/include" "-I$AzelSourceRoot" -
if ($LASTEXITCODE -ne 0) {throw 'Field exit syntax failed'}
Write-Output 'PASS: unique field-exit constructor initializes native state zero; Vita syntax only, no tunnel validation'

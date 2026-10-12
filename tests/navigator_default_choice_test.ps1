param([string]$AzelSourceRoot='E:/dev/Lagi/extern/Azel/AzelLib', [string]$VitaCompiler='E:/dev/VitaSDK/bin/arm-vita-eabi-g++.exe')
$ErrorActionPreference='Stop'
$root=Join-Path $PSScriptRoot '..'
$cmake=(Get-Content -Raw (Join-Path $root 'CMakeLists.txt')).Replace("`r`n","`n")
$source=(Get-Content -Raw (Join-Path $AzelSourceRoot 'field/field_a3/o_fld_a3.cpp')).Replace("`r`n","`n")
$old=[regex]::Match($cmake,'set\(navigator_default_old \[=\[([\s\S]*?)\]=\]\)').Groups[1].Value
$new=[regex]::Match($cmake,'set\(navigator_default_new \[=\[([\s\S]*?)\]=\]\)').Groups[1].Value
if (!$old -or !$new -or [regex]::Matches($source,[regex]::Escape($old)).Count -ne 1) {throw 'Default-choice anchor not unique'}
$source=$source.Replace($old,$new).Replace('#include "PDS.h"','#include "lagi/lagi_azel_upstream_prelude.h"')
$source | & $VitaCompiler -std=c++20 -fsyntax-only -x c++ -DLAGI_PLATFORM_VITA=1 "-I$root/include" "-I$AzelSourceRoot" "-I$AzelSourceRoot/field/field_a3" -
if ($LASTEXITCODE -ne 0) {throw 'Navigator default-choice syntax failed'}
Write-Output 'PASS: original default-choice index/clear/highlight branch Vita syntax; navigator pipeline still incomplete'

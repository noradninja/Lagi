param([Parameter(Mandatory=$true)][string]$AzelSourceRoot, [string]$VitaCompiler)
$ErrorActionPreference = 'Stop'
$cmake = Get-Content -Raw (Join-Path $PSScriptRoot '../CMakeLists.txt')
$native = Get-Content -Raw (Join-Path $AzelSourceRoot 'kernel/loadSavegameScreen.cpp')
# Select the REPLACE block before the marker and parse its literal bodies.
$marker = $cmake.IndexOf('        // LAGI_SAVE_OVERWRITE_CONFIRM')
$start = $cmake.LastIndexOf('   string(REPLACE', $marker)
$block = $cmake.Substring($start, $cmake.IndexOf('source_text "${source_text}")', $marker)-$start)
$literals = [regex]::Matches($block,'(?s)\[=\[(.*?)\]=\]')
if ($literals.Count -ne 2) { throw 'Expected two patch literals' }
$old = $literals[0].Groups[1].Value.Replace("`r`n","`n")
$new = $literals[1].Groups[1].Value.Replace("`r`n","`n")
$native = $native.Replace("`r`n","`n")
if (($native.Split(@($old),[StringSplitOptions]::None).Length-1) -ne 1) { throw 'Native patch target must occur exactly once' }
if ($new -notmatch 'return; // Do not fall through' -or
    $new -notmatch 'm10_status = 2;\s*return;' -or
    $new.IndexOf('buttons & 1') -gt $new.IndexOf('buttons & 6') -or
    $new -match 'saveData\(') { throw 'Unsafe confirmation transition' }
# Exercise the extracted production case-2 code, translated only at its C++
# syntax boundary. No file is opened or written by this test.
$case2 = $new.Substring($new.IndexOf('        if (buttons & 1)'))
$case2 = [regex]::Replace($case2, '//[^\r\n]*', '')
$case2 = $case2.Replace('buttons & 1','$buttons -band 1').Replace('buttons & 6','$buttons -band 6')
$case2 = $case2.Replace('playSystemSoundEffect(1);','').Replace('playSystemSoundEffect(0);','')
$case2 = $case2.Replace('*pThis->mC_pOperationResult = 1;','$result.operation = 1;')
$case2 = $case2.Replace('pThis->getTask()->markFinished();','$result.finished = $true;')
$case2 = $case2.Replace('pThis->m10_status = 3;','$result.state = 3;')
$case2 = $case2.Substring(0,$case2.LastIndexOf('    }'))
$run = [scriptblock]::Create('param($buttons,$result) ' + $case2)
foreach ($buttons in 0..7) {
    $result = @{state=2;operation=-1;finished=$false}
    & $run $buttons $result
    if ($buttons -band 1) {
        if (!$result.finished -or $result.operation -ne 1 -or $result.state -ne 2) { throw "Cancel failed buttons=$buttons" }
    } elseif ($buttons -band 6) {
        if ($result.finished -or $result.state -ne 3) { throw "Confirm failed buttons=$buttons" }
    } elseif ($result.finished -or $result.state -ne 2) { throw 'Idle must wait' }
}
Write-Output 'PASS: native patch matches once; extracted confirmation code waits, confirms and cancels for all 8 button combinations (no save writes)'
if ($VitaCompiler) {
    $patched = $native.Replace($old,$new).Replace('#include "PDS.h"','#include "lagi/lagi_azel_upstream_prelude.h"')
    $repo = Split-Path $PSScriptRoot -Parent
    $patched | & $VitaCompiler -std=c++20 -fsyntax-only -DLAGI_PLATFORM_VITA=1 -DLAGI_NEPTUNE_RENDERER=1 -I "$repo/include" -I "$repo/src/integration" -I $AzelSourceRoot -I "$AzelSourceRoot/kernel" -x c++ -
    if ($LASTEXITCODE -ne 0) { throw 'Patched native save/load source syntax check failed' }
    Write-Output 'PASS: patched native save/load source Vita syntax check (stdin; no source or save files written)'
}

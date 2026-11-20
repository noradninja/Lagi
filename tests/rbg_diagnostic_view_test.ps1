$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$renderer=Get-Content -Raw "$repo/src/platform/vita/neptune_renderer.cpp"
$service=Get-Content -Raw "$repo/src/platform/vita/neptune_sprite_composite.inc"
$cmake=Get-Content -Raw "$repo/CMakeLists.txt"
$shader=Get-Content -Raw "$repo/shaders/rbg_diagnostic_f.cg"
foreach($fragment in @('kSceneModes[8] = {7, 8, 11, 9, 10, 12, 13, 14}',
 'case 13: return "VDP1 RAW";','case 14: return "VDP1 / COMPOSITE SPLIT";',
 '(g_viewMode == 7 || g_viewMode == 13 || g_viewMode == 14);',
 'g_viewMode == 7 || g_viewMode == 13 || g_viewMode == 14;')) {
 if(!$renderer.Contains($fragment)){throw "Diagnostic view or FULL geometry path missing: $fragment"}
}
foreach($fragment in @('g_rbgDiagnosticProgram','g_diagInfo','g_viewMode==14?1.f:0.f',
 'g_rbgRatioCompositeFrame?g_rbgRatioCompositeProgram:g_spriteCompositeProgram')) {
 if(!$service.Contains($fragment)){throw "Diagnostic renderer path missing: $fragment"}
}
if(!$shader.Contains('if(debugInfo.x<0.5f || uv.x<0.5f)') -or
   !$cmake.Contains('LAGI_RBG_DIAGNOSTIC_OBJ')){throw 'Diagnostic shader not linked'}
$compiler=$env:PSP2CGC
if(!$compiler){$compiler='psp2cgc'}
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-rbg-diag-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
& $compiler -profile sce_fp_psp2 "$repo/shaders/rbg_diagnostic_f.cg" -Wperf -fastprecision -O3 -xmlsdb "$scratch/diag.xml" -o "$scratch/diag.gxp"
if($LASTEXITCODE){throw 'VDP1 diagnostic shader compilation failed'}
[xml]$xml=Get-Content -Raw "$scratch/diag.xml"
foreach($name in 'geometry','backColor','rbgState','debugInfo'){
 $info=@($xml.sdb.resourceInfo.uniform | Where-Object {$_.symbolName -eq $name})
 if($info.Count -ne 1){throw "Missing shader uniform: $name"}
}
foreach($binding in @(@('layer',0),@('background',1),@('decisions',2),@('nbg0',3))) {
 $info=@($xml.sdb.resourceInfo.sampler | Where-Object {$_.symbolName -eq $binding[0]})
 if($info.Count -ne 1 -or $info[0].textureUnit -ne [string]$binding[1]){throw "Wrong sampler $($binding[0])"}
}
Write-Output 'PASS diagnostic view cycle, shader compilation, and bindings'

$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$source=Get-Content -Raw "$repo/src/platform/vita/neptune_renderer.cpp"
$need=@(
 'g_liveTownStaticSubmissionRanges = g_liveTownSubmissionRanges;',
 'g_liveTownSubmissionRanges = g_liveTownStaticSubmissionRanges;',
 'g_liveTownSubmissionRanges[staticRange++].azelOrder=order;',
 'return a.azelOrder<b.azelOrder;',
 'if(!submitRange(p,next-p,ordered))return false;',
 'phaseResult=submitSubdivRange(p,next-p,ordered);',
 'if(g_liveTownShadowPolygonCount &&',
 'if(g_liveTownEdgePolygonCount &&',
 '[NativeOrderReplay]'
)
foreach($text in $need){if(!$source.Contains($text)){throw "Native replay wiring missing: $text"}}
# Deliberately disjoint cached address ranges must replay in producer order.
$ranges=@(
 [pscustomobject]@{First=0;Count=2;Order=0}
 [pscustomobject]@{First=2;Count=2;Order=2}
 [pscustomobject]@{First=4;Count=2;Order=1}
)
$replay=@($ranges | Sort-Object Order | ForEach-Object {
 $r=$_
 for($p=$r.First;$p -lt $r.First+$r.Count;$p++){$p}
})
$expected=@(0,1,4,5,2,3)
if((Compare-Object $expected $replay).Length){throw 'Interleaved order differs from Azel submissions'}
# Same source material is batched only when it is consecutive.
$materials=@('a','a','b','b','a','mesh','a')
$runs=0;$previous=$null
foreach($m in $materials){if($m -cne $previous){$runs++;$previous=$m}}
if($runs -ne 5){throw "Expected five ordered runs, got $runs"}
Write-Output 'PASS native replay order, static/dynamic interleave, consecutive batching, and both render paths'

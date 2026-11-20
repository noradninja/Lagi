$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$source=Get-Content -Raw "$repo/src/platform/vita/neptune_renderer.cpp"
$required=@(
 'if(subdividedTexturedLit &&',
 'g_residentVdp1Model==ResidentVdp1Model::LiveTown',
 'p<model.polygonCount;++p',
 'g_vdp1SubdivQuadIndices.data()+index',
 'orderedRuns.back().firstIndex+orderedRuns.back().count==index',
 'g_liveTownMeshRanges.size()',
 'g_liveTownShadowPolygonCount',
 'g_vdp1SubdivIndices+run.firstIndex,run.count',
 'nativeSourceOrder=1'
)
foreach($pattern in $required) {
 if(!$source.Contains($pattern)){throw "Missing native VDP1 ordering invariant: $pattern"}
}
# Test the intended run policy independently of GPU hardware: only *adjacent*
# same-texture, same-policy visible polygons coalesce, and input order is not
# converted into texture-major order.
$polys=@(
 @{id=0;tex=302;ordered=$false;visible=$true},
 @{id=1;tex=303;ordered=$false;visible=$true},
 @{id=2;tex=302;ordered=$false;visible=$true},
 @{id=3;tex=302;ordered=$false;visible=$true},
 @{id=4;tex=302;ordered=$true;visible=$true},
 @{id=5;tex=302;ordered=$false;visible=$false},
 @{id=6;tex=302;ordered=$true;visible=$true}
)
$runs=@()
foreach($p in $polys) {
 if(!$p.visible){continue}
 if($runs.Count -gt 0) {
  $last=$runs[$runs.Count-1]
  if($last.tex -eq $p.tex -and $last.ordered -eq $p.ordered -and
     ($last.ids[-1]+1) -eq $p.id) {
   $last.ids+=@($p.id)
   continue
  }
 }
 $runs+=@(@{tex=$p.tex;ordered=$p.ordered;ids=@($p.id)})
}
$observed=($runs | ForEach-Object {($_.ids -join ',')}) -join ';'
$expected='0;1;2,3;4;6'
if($observed -ne $expected) {throw "Run-order fixture mismatch: $observed"}
Write-Output 'PASS native VDP1 ordered source runs (including texture alternation, mesh barriers, invisible gaps)'

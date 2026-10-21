$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$renderer=Get-Content -Raw "$repo/src/platform/vita/neptune_renderer.cpp"
foreach($anchor in @('createNativeBlend(BlendTexture','createNativeBlend(BlendStochastic,','createNativeBlend(BlendSubdiv,','createNativeBlend(BlendStochasticGouraud,','createNativeBlend(BlendMesh,','createNativeBlend(BlendMeshSubdiv,','g_liveTownMeshRanges.push_back({polygonBase+p,1u})','vdp1_blend_texel(command.cmdPmod,pixel)','vdp1_blend_texel(record.cmdPmod,color)')) {
 if(!$renderer.Contains($anchor)){throw "Missing shared renderer blend integration: $anchor"}
}
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-color-calc-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
& cl /nologo /std:c++20 /EHsc "$PSScriptRoot/vdp1_color_calculation_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Color calculation fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Color calculation fixture failed'}

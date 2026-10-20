$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$renderer=Get-Content -Raw "$repo/src/platform/vita/neptune_renderer.cpp"
foreach($anchor in @('const bool mesh = (command.cmdPmod & 0x100u)!=0;','setStochasticGouraud(texture->width, texture->height, blended, mesh)','setSubdivMesh(mesh)','g_stochasticGouraudMeshParam','if (!mesh) setSubdivMesh(false);')) {
 if(!$renderer.Contains($anchor)){throw "Missing shared native sprite mesh integration: $anchor"}
}
foreach($name in @('stochastic_gouraud_f.cg','textured_gouraud_subdiv_f.cg')) {
 $source=Get-Content -Raw "$repo/shaders/$name"
 foreach($anchor in @('uniform float meshMode','floor(fragCoord.x)+floor(fragCoord.y)','discard;','base5 + correction5')) {
  if(!$source.Contains($anchor)){throw "Missing mesh/Gouraud operation in $name"}
 }
}
'PASS source contract: native mesh bit reaches stochastic and ordinary Gouraud sprite shaders without dropping texture/Gouraud arithmetic; GPU compilation and hardware remain separate gates'

$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$renderer=Get-Content -Raw "$repo/src/platform/vita/neptune_renderer.cpp"
foreach($anchor in @('const bool mesh = (command.cmdPmod & 0x100u)!=0;','setStochasticGouraud(texture->width, texture->height, blended, mesh)','g_gouraudMeshPrograms[blended?1:0]','g_meshStochasticTextureSizeParam')) {
 if(!$renderer.Contains($anchor)){throw "Missing shared native sprite mesh integration: $anchor"}
}
foreach($name in @('stochastic_gouraud_f.cg','textured_gouraud_subdiv_f.cg')) {
 $source=Get-Content -Raw "$repo/shaders/$name"
 foreach($anchor in @('#if LAGI_NATIVE_MESH','floor(fragCoord.x)+floor(fragCoord.y)','discard;','base5 + correction5')) {
  if(!$source.Contains($anchor)){throw "Missing mesh/Gouraud operation in $name"}
 }
}
if($renderer.Contains('setSubdivMesh(') -or $renderer.Contains('"meshMode"')){throw 'Full shader still pays mesh uniform setup'}
foreach($name in @('stochastic_gouraud_mesh_f.cg','textured_gouraud_subdiv_mesh_f.cg')) {
 if(!(Get-Content -Raw "$repo/shaders/$name").Contains('#define LAGI_NATIVE_MESH 1')){throw 'Mesh variant not compile-time specialized'}
}
'PASS source contract: native mesh bit reaches stochastic and ordinary Gouraud sprite shaders without dropping texture/Gouraud arithmetic; GPU compilation and hardware remain separate gates'

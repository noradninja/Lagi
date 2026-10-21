$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$shader=Get-Content "$root/shaders/rgba_present_f.cg" -Raw
if(([regex]::Matches($shader,'tex2D\(')).Count -ne 1 -or
    $shader -match '#include|discard|readBE16|sampleSceneNbg0') {
    throw 'Resolved RGBA display must remain a single sample without decode or cutout.'
}
$draw=Get-Content "$root/src/platform/vita/neptune_scene_nbg0.inc" -Raw
$display=$draw.Substring($draw.IndexOf('static void drawSceneNbg0Gpu'))
if($display -notmatch 'g_rgbaPresentInfo' -or $display -match 'g_vdp2InfoParam') {
    throw 'Cloud display must use the independent RGBA program uniforms.'
}
Write-Output 'PASS: independent single-sample premultiplied display contract; not GPU timing evidence'
$decode=Get-Content "$root/shaders/vdp2_nbg_f.cg" -Raw
if($decode -match 'sampleSceneNbg0|nbg0State|scene_nbg0.cginc' -or
    $draw -notmatch 'g_sceneNbg0ResolveProgram') {
    throw 'Cloud resolve must be independent of the frontend shader.'
}
$cloud=Get-Content "$root/shaders/scene_nbg0_f.cg" -Raw
if($cloud -notmatch 'return sampleSceneNbg0\(raw,vTexcoord,nbg0State,rowData\)' -or $cloud -match 'vdp2Info') {
    throw 'Cloud shader must call the tested production decoder without a frontend mode selector.'
}
Write-Output 'PASS: cloud resolve program is independent of frontend layouts 0, 1 and 2'

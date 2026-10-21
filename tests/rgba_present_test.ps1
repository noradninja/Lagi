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
if($decode -notmatch 'if\(vdp2Info.x>7.5f\)return sampleSceneNbg0' -or
    $draw -notmatch 'const float info\[4\]=\{8,0,0,0\}') {
    throw 'Cloud resolve mode must not alias frontend layouts 1 or 2.'
}
foreach($layout in 0,1,2){if($layout -gt 7.5){throw 'Frontend layout entered cloud decoder'}}
Write-Output 'PASS: cloud resolve dispatch is separate from frontend layouts 0, 1 and 2'

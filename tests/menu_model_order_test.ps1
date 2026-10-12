$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$source=Get-Content -Raw (Join-Path $repo 'src/platform/vita/neptune_renderer.cpp')
$marker=$source.IndexOf('// Menu NBG1/0 priorities 3/4 are behind BOTH')
$end=$source.IndexOf('// All native scenes share',$marker)
if ($marker -lt 0 -or $end -le $marker) {throw 'Menu composition boundary missing'}
$branch=$source.Substring($marker,$end-$marker)
$backing=$branch.IndexOf('drawAzelMenuVdp2Gpu();')
$model=$branch.IndexOf('submit_vdp1_model(model, drawState)')
$sprites=$branch.IndexOf('drawPublishedVdp1Ui(')
$text=$branch.IndexOf('drawAzelMenuVdp2Gpu(true)')
if ($backing -lt 0 -or $model -le $backing -or $sprites -le $model -or $text -le $sprites -or
    [regex]::Matches($branch,'drawAzelMenuVdp2Gpu\(\);').Count -ne 1) {throw 'Menu backing/model/decorations/text order incorrect'}
foreach($face in @('Front','Back')) {
 foreach($call in @("sceGxmSet${face}DepthFunc(g_probeContext, SCE_GXM_DEPTH_FUNC_ALWAYS)","sceGxmSet${face}DepthWriteEnable(g_probeContext, SCE_GXM_DEPTH_WRITE_DISABLED)")) {
  $state=$branch.IndexOf($call)
  if ($state -lt 0 -or $state -ge $backing) {throw 'Menu backing can contaminate model depth'}
 }
}
if ($branch -notmatch 'roomAuthenticCameraMode && \(g_vdp2MenuId != 0u \|\| g_vdp2SaveLayout\)') {throw 'Menu-only backing gate changed'}
if ($source -notmatch 'g_profileComposeUs = menuBackingUs \+') {throw 'Early menu backing cost missing from composition profiling'}
Write-Output 'PASS: menu-only backing/model/decorations/text source order, no backing depth writes, one backing pass and preserved profiling (not GPU execution)'

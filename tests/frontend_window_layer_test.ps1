$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$s=Get-Content -Raw "$repo/src/platform/vita/neptune_renderer.cpp"
if(!$s.Contains('rbg.layerEnable=state.bgon;')){throw 'Live native layer enable snapshot lost'}
$start=$s.IndexOf('static bool renderMovieFrame()' + "`r`n" + '{')
if($start -lt 0){throw 'Frontend renderer missing'}
$body=$s.Substring($start)
$tiles=$body.IndexOf('if (submitted && frontendRbg && (g_movieRbg0.layerEnable&2u))')
$draw=$body.IndexOf('drawAzelVdp2Nbg1Gpu();',$tiles)
$text=$body.IndexOf('drawAzelVdp2TextLayerGpu(movieOutputWidth, movieOutputHeight);')
$cursor=$body.IndexOf('drawPublishedVdp1Ui(Vdp1UiPass::FrontendOverlay,&g_frontendSpriteSlot);')
if($tiles -lt 0 -or $draw -lt $tiles -or $text -lt $draw -or $cursor -lt $text){throw 'Window tiles must follow native enable and precede text and cursor'}
Write-Output 'PASS frontend NBG1 live enable and window/text/cursor ordering contracts; hardware pixels pending'

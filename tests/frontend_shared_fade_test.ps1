$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$source=Get-Content -Raw "$repo/src/platform/vita/neptune_renderer.cpp"
if(!$source.Contains('return enable==0x7fu && (select==0u || select==0x7fu);')){throw 'Shared native bank predicate changed'}
if(!$source.Contains('if (!frontend || !frontendHasSharedColorOffset())')){throw 'RBG fade must not be doubled'}
$overlay=$source.IndexOf('drawPublishedVdp1Ui(Vdp1UiPass::FrontendOverlay,&g_frontendSpriteSlot)')
$fade=$source.IndexOf('if (d5FrontEnd && frontendHasSharedColorOffset())',$overlay)
if($overlay -lt 0 -or $fade -lt $overlay){throw 'Shared fade must follow frontend sprites'}
Write-Output 'PASS shared native fade bank and composition ordering contract; device validation pending'

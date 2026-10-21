$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$source=Get-Content -Raw "$repo/src/platform/vita/neptune_renderer.cpp"
if(!$source.Contains('return lagi::shared_color_offset(enable,select,')){throw 'Shared native bank predicate missing'}
if(!$source.Contains('if (!frontend || !frontendHasSharedColorOffset())')){throw 'RBG fade must not be doubled'}
$overlay=$source.IndexOf('drawPublishedVdp1Ui(Vdp1UiPass::FrontendOverlay,&g_frontendSpriteSlot)')
$fade=$source.IndexOf('if (d5FrontEnd && frontendHasSharedColorOffset())',$overlay)
if($overlay -lt 0 -or $fade -lt $overlay){throw 'Shared fade must follow frontend sprites'}
Write-Output 'PASS shared native fade bank and composition ordering contract; device validation pending'
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-shared-fade-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
& cl /nologo /EHsc /std:c++17 "/I$repo/include" "$PSScriptRoot/shared_color_offset_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Shared offset fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Shared offset fixture failed'}
Write-Output 'PASS identical and distinct native A/B offset banks across signed-nine-bit fade range'

param([string]$ScratchRoot='../ui-cache-index-test')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $repo $ScratchRoot))
New-Item -ItemType Directory -Force $scratch|Out-Null
& cl /nologo /O2 /EHsc /std:c++20 "/I$repo/include" "$PSScriptRoot/ui_texture_cache_index_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Sprite cache index fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Sprite cache index fixture failed'}
$source=Get-Content -Raw "$repo/src/platform/vita/neptune_renderer.cpp"
$begin=$source.IndexOf('static GpuMode1Texture* findOrUploadVdp1UiTexture(')
$end=$source.IndexOf('static float menuCanvasHalfWidth()',$begin)
if($begin -lt 0 -or $end -le $begin){throw 'Cache source boundary changed'}
$body=$source.Substring($begin,$end-$begin)
if($body.Contains('for (auto& entry : g_vdp1UiTextureCache)') -or
 !$body.Contains('g_vdp1UiTextureIndex.find(cacheKey,cacheIndex)') -or
 ([regex]::Matches($body,'g_vdp1UiTextureIndex.insert\(cacheKey,').Count -ne 2)){
 throw 'Production cache lookup/insertion routing changed'
}
if(!$source.Contains('g_profileSprites={};') -or !$source.Contains('spriteSlotSkips=%u')){throw 'Sprite profiler reset or telemetry missing'}
Write-Output 'PASS: production indexed lookups, both successful insertion paths and profiler reset.'

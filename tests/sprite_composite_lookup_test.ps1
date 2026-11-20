$ErrorActionPreference='Stop'
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-composite-lookup-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
& cl /nologo /O2 /std:c++20 /EHsc "$PSScriptRoot/sprite_composite_lookup_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Composite lookup fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Composite lookup fixture failed'}
$repo=Split-Path $PSScriptRoot -Parent
$shader=Get-Content -Raw "$repo/shaders/sprite_composite_f.cg"
if(!$shader.Contains('decisions:TEXUNIT2') -or !$shader.Contains('floor((a*(31-r)+b*(r+1))/32)')){throw 'Lookup or exact RGB555 blending lost'}
$empty=$shader.IndexOf('if(code<128.0f)return float4(dst.rgb,1);')
$lookup=$shader.IndexOf('tex2D(decisions,')
if($empty -lt 0 -or $lookup -le $empty){throw 'Empty sprite pixels must skip the decision fetch'}
$service=Get-Content -Raw "$repo/src/platform/vita/neptune_sprite_composite.inc"
if(!$service.Contains('spriteCompositeLookup(g_spriteColor,i)') -or !$service.Contains('SetFragmentTexture(g_probeContext,2,&g_spriteLookupTexture)') -or !$service.Contains('SetMagFilter(&g_spriteLookupTexture,SCE_GXM_TEXTURE_FILTER_POINT)')){throw 'Published-state point-sampled lookup wiring lost'}

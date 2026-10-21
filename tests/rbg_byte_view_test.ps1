$ErrorActionPreference='Stop'
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-rbg-byte-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
& cl /nologo /O2 /std:c++20 /EHsc "$PSScriptRoot/rbg_byte_view_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Byte view fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Byte view fixture failed'}
$repo=Split-Path $PSScriptRoot -Parent
$renderer=Get-Content -Raw "$repo/src/platform/vita/neptune_renderer.cpp"
if(!$renderer.Contains('&g_sceneVdp2ByteTexture,g_sceneVdp2GpuRaw,') -or !$renderer.Contains('SCE_GXM_TEXTURE_FORMAT_U8_RRRR,2048u,258u,0)') -or !$renderer.Contains('SetFragmentTexture(g_probeContext,1,&g_sceneVdp2ByteTexture)')){throw 'Shared byte view descriptor/binding lost'}
$shader=Get-Content -Raw "$repo/shaders/vdp2_rbg0_f.cg"
if(!$shader.Contains('rawBytes:TEXUNIT1') -or !$shader.Contains('min(3.0f,floor(dotAddress-byteGroup*4.0f+0.5f))') -or !$shader.Contains('dotColor = readByte(raw, dotAddress);')){throw 'Byte view or packed fallback changed'}

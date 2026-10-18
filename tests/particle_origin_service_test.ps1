param([string]$ScratchRoot='../particle-origin-service-test',[string]$Build='build-battle-dev')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $repo $ScratchRoot))
New-Item -ItemType Directory -Force $scratch | Out-Null
$mocks=Get-Content -Raw "$PSScriptRoot/collection_burst_service_test.cpp"
$mockEnd=$mocks.IndexOf('#include "collection_burst_body.inc"')
if($mockEnd -lt 0){throw 'Shared particle fixture boundary changed'}
[IO.File]::WriteAllText("$scratch/particle_mocks.inc",$mocks.Substring(0,$mockEnd))
$source=Get-Content -Raw "$repo/src/integration/lagi_vdp1_animated_quad_vita.cpp"
$pool=Get-Content -Raw "$repo/$Build/azel_upstream/field_fieldParticlePool.cpp"
if($pool -notmatch 'lagiDrawPoolParticle\(&pEntry->m2C_animQuad, &pEntry->m0_position,\s*pEntry->m18_velocityScaleX, pEntry->m1C_velocityScaleY\)'){
 throw 'Generated pool billboard does not forward native scale and sorting bias'
}
$begin=$source.IndexOf('static int drawQuadInternal(')
$end=$source.IndexOf('int drawProjectedParticle(',$begin)
if($begin -lt 0 -or $end -le $begin){throw 'Production particle body boundary changed'}
$colorBegin=$source.IndexOf('int lagiDrawScaledParticleWithColorTable(')
$colorEnd=$source.IndexOf('// Native ray service:',$colorBegin)
if($colorBegin -lt 0 -or $colorEnd -le $colorBegin){throw 'Production scaled-color service boundary changed'}
[IO.File]::WriteAllText("$scratch/particle_body.inc",$source.Substring($begin,$end-$begin)+$source.Substring($colorBegin,$colorEnd-$colorBegin))
$poolBegin=$source.IndexOf('int lagiDrawPoolParticle(')
$poolEnd=$source.IndexOf('int drawProjectedParticleWithGouraud(',$poolBegin)
if($poolBegin -lt 0 -or $poolEnd -le $poolBegin){throw 'Production pool projection boundary changed'}
[IO.File]::AppendAllText("$scratch/particle_body.inc",$source.Substring($poolBegin,$poolEnd-$poolBegin))
& cl /nologo /O2 /std:c++20 /EHsc "/I$scratch" "/I$repo/include" "$PSScriptRoot/particle_origin_service_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Particle origin service test compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Particle origin service test failed'}

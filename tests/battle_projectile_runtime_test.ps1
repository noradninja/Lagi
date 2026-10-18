param([string]$ScratchRoot='../battle-projectile-runtime-test',[string]$Build='build-battle-dev')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $repo $ScratchRoot))
New-Item -ItemType Directory -Force $scratch | Out-Null
$source=Get-Content -Raw "$repo/src/integration/lagi_battle_projectile_vita.cpp"
$begin=$source.IndexOf('// Content-keyed immutable runtime descriptor cache.')
if($begin -lt 0){throw 'Projectile runtime fixture boundary changed'}
[IO.File]::WriteAllText("$scratch/projectile_runtime.inc",$source.Substring($begin))
& cl /nologo /O2 /EHsc /std:c++20 "/I$scratch" "/I$repo/src/integration" "$PSScriptRoot/battle_projectile_runtime_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Projectile runtime fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Projectile runtime fixture failed'}
$dispatch=Get-Content -Raw "$repo/$Build/azel_upstream/battle_BTL_A3_BTL_A3_data.cpp"
foreach($route in @('lagiCreateUrchinProjectileEffect(pParent,false,true)','lagiCreateUrchinProjectileEffect(pParent,false,false)','lagiCreateUrchinProjectileEffect(pParent,true,true)','lagiCreateUrchinProjectileEffect(pParent,true,false)','lagiCreateUrchinSingleProjectile(pParent)')){
 if(!$dispatch.Contains($route)){throw "Missing live native projectile route: $route"}
}
$effects=Get-Content -Raw "$repo/$Build/azel_upstream/battle_BTL_A3_BTL_A3_urchinEffects.cpp"
if($effects.Contains('(u8*)pParent + 8') -or !$effects.Contains('pNewTask->m0_positionPtr = lagiUrchinEffectPosition(pParent);')){throw 'Raw Saturn parent position remains in A3 effect factory'}

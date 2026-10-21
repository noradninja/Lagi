param([string]$Build='build-battle-dev',[string]$ScratchRoot='../shootable-cart-test',[string]$DiscImage='')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $repo $ScratchRoot))
New-Item -ItemType Directory -Force $scratch | Out-Null
$source=Get-Content -Raw "$repo/$Build/azel_upstream/field_field_a3_a3_dynamic_mine_cart.cpp"
if(!$source.Contains('#include "lagi_a3_shootable_cart_update.inc"')){throw 'Live source lacks recovered cart state machine'}
$begin=$source.IndexOf('void A3_0_Obj3_LCSCallback(')
$end=$source.IndexOf('void create_A3_0_Obj3(',$begin)
if($begin -lt 0 -or $end -le $begin){throw 'Cart callback boundary changed'}
[IO.File]::WriteAllText("$scratch/cart_callback.inc",$source.Substring($begin,$end-$begin))
$smoke=Get-Content -Raw "$repo/$Build/azel_upstream/field_field_a3_o_fld_a3.cpp"
$begin=$smoke.IndexOf('struct sSmokePufTask :')
$end=$smoke.IndexOf('// 0605e928',$begin)
if($begin -lt 0 -or $end -le $begin){throw 'Cart smoke task boundary changed'}
[IO.File]::WriteAllText("$scratch/cart_smoke.inc",$smoke.Substring($begin,$end-$begin))
foreach($anchor in @('case 1:','case 2:','case 3:','mainGameState.getBit(0x93 * 8 + 4)','pNewTask->m_UpdateMethod = &s_A3_0_Obj3::Update2;','pNewTask->mC = sSaturnPtr({ 0x609271C, gFLD_A3 }) + (r12 - 3) * 4;')){
 if(!$source.Contains($anchor)){throw "Cart variant/persistence wiring changed: $anchor"}
}
& cl /nologo /O2 /std:c++20 /EHsc "/I$scratch" "/I$repo/src/integration" "$PSScriptRoot/shootable_cart_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Shootable cart test compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Shootable cart test failed'}
if($DiscImage){
 $stream=[IO.File]::OpenRead($DiscImage)
 try {
  $anchors=@{0x0605E4FA=0x8801;0x0605E4FE=0x8802;0x0605E508=0x53E7;0x0605E510=0x53E9;
   0x0605E536=0x3233;0x0605E556=0x3123;0x0605E568=0xE06C;0x0605E56A=0x03EC;
   0x0605E584=0x201A;0x0605E594=0x3423;0x0605E598=0x201A;
   0x0605E5AC=0x1305;0x0605E5B0=0x53E7;0x0605E5B8=0x53E9;
   0x0605E602=0xE06C;0x0605E606=0x0414;0x0605E690=0x0605E928}
  foreach($address in $anchors.Keys){
   $offset=$address-0x06054000
   $stream.Position=([long]244544+[int][Math]::Floor($offset/2048))*2352+16+($offset%2048)
   $width=2;if($anchors[$address] -gt 65535){$width=4}
   $actual=0L;for($i=0;$i -lt $width;$i++){$actual=($actual-shl 8)-bor $stream.ReadByte()}
   if($actual -ne $anchors[$address]){throw ('Original cart anchor changed at {0:X8}' -f $address)}
  }
 } finally {$stream.Dispose()}
 Write-Output 'PASS original FLD_A3 shot-cart motion, endpoint, callback and smoke anchors; not Vita acceptance.'
}

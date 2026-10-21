param([Parameter(Mandatory=$true)][string]$DiscImage,[string]$BuildDirectory='build-battle-dev')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$disc=[IO.File]::OpenRead($DiscImage)
try {
 function Read-Native([long]$ea,[int]$size){$v=0L;for($i=0;$i -lt $size;$i++){$o=$ea+$i-0x06054000;$disc.Position=(258333+[long][Math]::Floor($o/2048))*2352+16+($o%2048);$v=($v-shl 8)-bor $disc.ReadByte()};return $v}
 foreach($base in @(0x060A9814,0x060A999C)) {
  for($i=0;$i -lt 14;$i++) {
   $ea=$base+$i*28
   $last=if($i -eq 13){1}else{0}
   $delay=if($base -eq 0x060A999C -or $i -eq 13){4}else{2}
   $mode=if($base -eq 0x060A999C){0x188}else{0x88}
   if((Read-Native $ea 1) -ne $last -or (Read-Native ($ea+1) 1) -ne $delay -or (Read-Native ($ea+4) 2) -ne $mode){throw 'Native particle animation frame/delay/material contract differs'}
  }
 }
}finally{$disc.Dispose()}
$generated=Get-Content -Raw "$repo/$BuildDirectory/azel_upstream/battle_BTL_A3_2_Arachnoth.cpp"
if(!$generated.Contains('const auto& cachedQuads = lagiBattleParticleFrames(spriteDataEA)')){throw 'Immutable frame adapter absent'}
$step=Get-Content -Raw "$repo/extern/Azel/AzelLib/kernel/vdp1AnimatedQuad.cpp"
$begin=$step.IndexOf('int sGunShotTask_UpdateSub4(');$end=$step.IndexOf('int clipQuad(', $begin)
if($begin -lt 0 -or $end -lt $begin){throw 'Native frame-step boundary changed'}
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-particle-frames-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
[IO.File]::WriteAllText("$scratch/step.inc",$step.Substring($begin,$end-$begin))
& cl /nologo /std:c++20 /EHsc "/I$scratch" "$PSScriptRoot/battle_particle_frames_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Particle frames fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Particle frames fixture failed'}

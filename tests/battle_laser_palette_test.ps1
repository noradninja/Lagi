param([string]$BattleBuild='build-battle-dev',[string]$ScratchRoot='../battle-laser-palette-test',[string]$DiscImage='')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $repo $ScratchRoot))
New-Item -ItemType Directory -Force $scratch | Out-Null
$source=Get-Content -Raw "$repo/$BattleBuild/azel_upstream/battle_berserk_laser.cpp"
$begin=$source.IndexOf('static void renderBeamTrailSegments(')
$end=$source.IndexOf('// BTL_A3::06096ca0',$begin)
if($begin -lt 0 -or $end -le $begin){throw 'Generated laser draw boundary changed'}
[IO.File]::WriteAllText("$scratch/laser_draw.inc",$source.Substring($begin,$end-$begin))
& cl /nologo /O2 /std:c++20 /EHsc "/I$scratch" "$PSScriptRoot/battle_laser_palette_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Laser palette fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Laser palette fixture failed'}
if($DiscImage){
 $stream=[IO.File]::OpenRead($DiscImage)
 try {
  # Original 0609DA3E..56 loads allocation and adds it to COLR and SRCA.
  foreach($pair in @(@(0x0609DA3E,0x85EA),@(0x0609DA44,0x85C4),@(0x0609DA46,0x30AC),@(0x0609DA52,0x85C2),@(0x0609DA56,0x360C))){
   $offset=$pair[0]-0x06054000
   $stream.Position=([long]258144+[int][Math]::Floor($offset/2048))*2352+16+($offset%2048)
   $actual=($stream.ReadByte()-shl 8)-bor $stream.ReadByte()
   if($actual -ne $pair[1]){throw ('Original laser palette anchor changed at {0:X8}' -f $pair[0])}
  }
 } finally {$stream.Dispose()}
 Write-Output 'PASS original laser palette relocation anchors; not Vita visual acceptance.'
}

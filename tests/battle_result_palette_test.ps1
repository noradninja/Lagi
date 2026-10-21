param([Parameter(Mandatory=$true)][string]$DiscImage,[string]$BuildDirectory='build-battle-dev')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$disc=[IO.File]::OpenRead($DiscImage)
try {
 function Read-Native([long]$ea,[int]$size){$v=0L;for($i=0;$i -lt $size;$i++){$o=$ea+$i-0x06054000;$disc.Position=(258144+[long][Math]::Floor($o/2048))*2352+16+($o%2048);$v=($v-shl 8)-bor $disc.ReadByte()};return $v}
 if((Read-Native 0x0606E858 2) -ne 0x50D2 -or (Read-Native 0x0606E85A 2) -ne 0xC90F){throw 'Native result palette load differs'}
 for($i=0;$i -lt 5;$i++){
  $expected=12;if($i -eq 0){$expected=13}
  if((Read-Native (0x060ABA28+$i*16) 4) -ne $expected){throw 'Native result label palette differs'}
 }
}finally{$disc.Dispose()}
$source=Get-Content -Raw "$repo/$BuildDirectory/azel_upstream/battle_battleResultScreenProgressiveText.cpp"
$begin=$source.IndexOf('void battleResultScreenProgressiveText_printString(')
$end=$source.IndexOf('// BTL_A3::060709c2',$begin)
if($begin -lt 0 -or $end -lt 0){throw 'Result text function missing'}
$function=$source.Substring($begin,$end-$begin)
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-result-palette-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
[IO.File]::WriteAllText("$scratch/production.inc",$function)
& cl /nologo /std:c++20 /EHsc "/I$scratch" "$PSScriptRoot/battle_result_palette_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Result palette fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Result palette fixture failed'}

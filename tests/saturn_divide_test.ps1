param([string]$DiscImage)
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
if($DiscImage){
 $disc=[IO.File]::OpenRead($DiscImage)
 try {
  $expected=@(0x354D,0x9109,0x1113,0x040A,0x1160,0x051A,0x1144,0x1155,0xE400,0x5017,0x000B,0x1143,0xFF00)
  for($i=0;$i -lt $expected.Count;$i++){
   $word=0
   for($byte=0;$byte -lt 2;$byte++){
    $offset=0x06035686+$i*2+$byte-0x06006000
    $disc.Position=(58+[long][Math]::Floor($offset/2048))*2352+16+$offset%2048
    $word=($word-shl 8)-bor $disc.ReadByte()
   }
   if($word -ne $expected[$i]){throw 'Original setDividend instruction contract changed'}
  }
  'PASS original MAIN 06035686 signed product, DIVU register writes and quotient read'
 }finally{$disc.Dispose()}
}
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-divide-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
& cl /nologo /std:c++20 /EHsc "/I$repo/include" "$PSScriptRoot/saturn_divide_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Saturn divide test compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Saturn divide fixture failed'}

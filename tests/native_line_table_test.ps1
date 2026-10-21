param([Parameter(Mandatory=$true)][string]$DiscImage)
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$disc=[IO.File]::OpenRead($DiscImage)
try {
 function Read-Word($lba,$base,$address) {
  $value=0
  for($i=0;$i -lt 2;++$i) {
   $offset=$address+$i-$base
   $disc.Position=($lba+[math]::Floor($offset/2048))*2352+16+($offset%2048)
   $b=$disc.ReadByte();if($b -lt 0){throw 'Short disc read'}
   $value=($value-shl 8)-bor $b
  }
  return $value
 }
 if((Read-Word 218450 0x6054000 0x6054F26) -ne 0xC0){throw 'Native E011 block count differs'}
 if((Read-Word 58 0x6006000 0x6032510) -ne 0x8166){throw 'Native DMA block-count store differs'}
} finally {$disc.Dispose()}
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-line-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
& cl /nologo /EHsc /std:c++17 "/I$repo/include" "$PSScriptRoot/native_line_table_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Line-table test compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Line-table test failed'}

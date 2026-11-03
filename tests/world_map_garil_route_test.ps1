param([Parameter(Mandatory=$true)][string]$DiscImage)
$ErrorActionPreference='Stop'
$disc=[IO.File]::OpenRead((Resolve-Path -LiteralPath $DiscImage).Path)
try {
 function Read-Native([long]$ea,[int]$count,[long]$base,[long]$lba){
  $value=0L
  for($i=0;$i-lt$count;$i++){
   $offset=$ea+$i-$base
   $disc.Position=($lba+[long][Math]::Floor($offset/2048))*2352+16+($offset%2048)
   $b=$disc.ReadByte()
   if($b-lt0){throw 'Short native route read'}
   $value=($value-shl8)-bor$b
  }
  return $value
 }
 function Read-CommonString([long]$ea){
  if($ea-lt0x200000-or$ea-ge0x220000){throw 'COMMON string outside expected image'}
  $text=''
  for($i=0;$i-lt80;$i++){
   $b=Read-Native ($ea+$i) 1 0x200000 182
   if(!$b){return $text}
   $text += [char]$b
  }
  throw 'Unterminated COMMON route string'
 }
 $entry=0x06059688+6*0x24
 if((Read-Native $entry 2 0x06054000 244259)-ne0x6C-or
    (Read-Native ($entry+2) 2 0x06054000 244259)-ne0x56-or
    (Read-Native ($entry+4) 1 0x06054000 244259)-ne3-or
    (Read-Native ($entry+5) 1 0x06054000 244259)-ne0){throw 'Garil destination record differs'}
 $nameEa=Read-Native ($entry+0x20) 4 0x06054000 244259
 if($nameEa-lt0x06054000-or$nameEa-ge0x06059ED4){throw 'Garil label outside WORLD'}
 $name=''
 for($i=0;$i-lt32;$i++){
  $b=Read-Native ($nameEa+$i) 1 0x06054000 244259
  if(!$b){break}
  $name += [char]$b
 }
 if($name-ne'Garil Desert'){throw 'Garil destination label differs'}
 $modeEa=0x212EAC+0x56*2
 if((Read-Native $modeEa 1 0x200000 182)-ne3-or
    (Read-Native ($modeEa+1) 1 0x200000 182)-ne3){throw 'Garil status dispatch differs'}
 # Native field definition is five longwords, not the WORLD destination stride.
 $field=0x208C58+3*20
 $program=Read-CommonString (Read-Native ($field+4) 4 0x200000 182)
 $font=Read-CommonString (Read-Native ($field+8) 4 0x200000 182)
 if($program-ne'FLD_A5.PRG'-or$font-ne'FLD_A5.FNT'){throw 'Garil native field assets differ'}
 Write-Output 'PASS original Garil entry6 gate006C -> status56 -> mode3/field3 -> FLD_A5.PRG/FNT; no story bits changed; not a live travel test'
} finally {$disc.Dispose()}

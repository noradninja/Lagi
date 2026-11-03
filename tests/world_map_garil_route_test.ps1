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
 # Resolve original PC-relative loads, rather than assuming the neighboring
 # synchronous file wrapper is the one used by WORLD preparation.
 function Assert-MainLiteral([long]$ea,[long]$expected){
  $op=Read-Native $ea 2 0x06006000 58
  $literal=(($ea+4)-band -4)+4*($op-band255)
  if(($op-shr12)-ne0xD-or(Read-Native $literal 4 0x06006000 58)-ne$expected){
   throw ('WORLD preparation literal differs at {0:X8}'-f$ea)
  }
 }
 Assert-MainLiteral 0x06011E0C 0x208C58
 Assert-MainLiteral 0x06011E24 0x060100F2
 Assert-MainLiteral 0x06011E34 0x06054000
 Assert-MainLiteral 0x06011E3A 0x060100F2
 Assert-MainLiteral 0x06030100 0x2166E4
 Assert-MainLiteral 0x06030104 0x06054000
 Assert-MainLiteral 0x0603010C 0x2165D8
 Assert-MainLiteral 0x06030114 0x060100F2
 foreach($pair in @(
  @(0x06011E14,0x4E08),@(0x06011E16,0x3E3C),@(0x06011E18,0x4E08), # field stride20
  @(0x06011E1C,0x50E2),@(0x06011E22,0xE600),@(0x06011E26,0x6563),
  @(0x06011E2A,0x54E2), # font filename +8, destination/relocation0
  @(0x06011E2C,0x50E1),@(0x06011E32,0xE600),@(0x06011E36,0x54E1), # PRG +4
  @(0x06030108,0x4400),@(0x0603010A,0x034C), # town-pair stride2, signed index
  @(0x06030112,0x4408),@(0x06030116,0x4408),@(0x06030118,0x044E), # town stride16
  @(0x06030102,0xE600),@(0x0603011A,0x432B))) {
  if((Read-Native $pair[0] 2 0x06006000 58)-ne$pair[1]){throw 'WORLD preparation addressing differs'}
 }
 Write-Output 'PASS original Garil entry6 gate006C -> status56 -> mode3/field3 -> FLD_A5.PRG/FNT; no story bits changed; not a live travel test'
 Write-Output 'PASS native field/town preparation tables and MAIN060100F2 call sites; file-service semantics remain under recovery'
} finally {$disc.Dispose()}

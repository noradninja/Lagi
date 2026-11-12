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
 # Disc 1 FLD_A5.PRG is LBA244678, 316576 bytes. Its entry owns a
 # separate script table; do not mistake a linked A3 shared service for A5 data.
 foreach($literal in @(@(0x06054170,0x0609D4A8),@(0x06054174,0x06089A38),
                       @(0x06054184,0x06071708),@(0x0609D50C,0x06061955))) {
  if((Read-Native $literal[0] 4 0x06054000 244678)-ne$literal[1]){throw 'Garil native startup ownership differs'}
 }
 if((Read-Native 0x06061955 1 0x06054000 244678)-ne27-or
    (Read-Native 0x06061956 2 0x06054000 244678)-ne60-or
    (Read-Native 0x06061958 4 0x06054000 244678)-ne0x060655C9-or
    (Read-Native 0x0606195C 1 0x06054000 244678)-ne1){throw 'Garil open-field entry label script differs'}
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
 # A5 startup chooses a distinct Floater task and reloads the saved rider.
 # Its previous-type guard owns the later switch to Edge and Gash.
 foreach($pair in @(
  @(0x06072F40,0x0607C2D0), @(0x0607C430,0x0604A6BC),
  @(0x0607C43C,0x0600B444), @(0x0607C444,0x0609FDA4),
  @(0x0607C448,0x0609FDB4), @(0x0609FDA8,0x0607BA6C),
  @(0x0607BAF0,0x0600B412), @(0x0607BAF4,0x0600B444),
  @(0x0607BAF8,0x0607B8F0), @(0x0607BAFC,0x0607BECC)
 )) {
  if((Read-Native $pair[0] 4 0x06054000 244678)-ne$pair[1]){
   throw ('Garil rider transition address differs at {0:X8}'-f$pair[0])
  }
 }
 foreach($pair in @(
  @(0x0607C2E6,0xE03E), @(0x0607C2EA,0x8801), @(0x0607C2EC,0x8B14),
  @(0x0607C2EE,0x84E1), @(0x0607C2F6,0x84E2), @(0x0607C2FE,0x84E3),
  @(0x0607C30C,0x81E8), @(0x0607C312,0x81EA), @(0x0607C316,0x0D34), @(0x0607BA84,0x901B), @(0x0607BABE,0x0100),
  @(0x0607BA88,0x03EE), @(0x0607BA8A,0x3320), @(0x0607BA8C,0x8938),
  @(0x0607BA9E,0x8808), @(0x0607BAA0,0x892E),
  @(0x0607BAA6,0xE401), @(0x0607BAAC,0xE402),
  @(0x0607B96C,0xE406), @(0x0607B972,0xE407)
 )) {
  if((Read-Native $pair[0] 2 0x06054000 244678)-ne$pair[1]){
   throw ('Garil rider transition instruction differs at {0:X8}'-f$pair[0])
  }
 }
 Write-Output 'PASS native A5 saved rider reload, Floater type-change guard, and rider/callback transitions'

 Write-Output 'PASS original Garil entry6 gate006C -> status56 -> mode3/field3 -> FLD_A5.PRG/FNT; no story bits changed; not a live travel test'
 Write-Output 'PASS native field/town preparation tables and MAIN060100F2 call sites; file-service semantics remain under recovery'
 Write-Output 'PASS original A5 entry script table and subfield0 location-label script; not an A5 execution or gameplay test'
} finally {$disc.Dispose()}

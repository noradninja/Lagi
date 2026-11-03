param([Parameter(Mandatory=$true)][string]$DiscImage)
$ErrorActionPreference='Stop'
$disc=[IO.File]::OpenRead((Resolve-Path -LiteralPath $DiscImage).Path)
try {
 function Read-World([long]$ea,[int]$count) {
  $value=0L
  for($i=0;$i -lt $count;$i++) {
   $offset=$ea+$i-0x06054000
   $disc.Position=(244259+[long][Math]::Floor($offset/2048))*2352+16+($offset%2048)
   $b=$disc.ReadByte()
   if($b -lt 0){throw 'Short WORLD read'}
   $value=($value -shl 8)-bor $b
  }
  return $value
 }
 # WORLD 060565C0 modulates the current rotation coefficient table. It is
 # not a newly generated NBG0 line-scroll table. Check native instructions,
 # not just the routine address or a translated helper's name.
 $words=@(
  @(0x060565D4,0x9831), # mov.w count literal -> r8
  @(0x060565EA,0x0DDE), # load selected table pointer
  @(0x060565EC,0xE054), # phase at environment +54
  @(0x060565F4,0x4529), # phase >>16
  @(0x060565F8,0x25A9), # mask sine index
  @(0x060565FC,0x055E), # sine table lookup
  @(0x06056602,0x04EE), # amplitude at +50, multiply delay slot
  @(0x06056606,0x359C), # add fixed-point unity
  @(0x0605660A,0x64D2), # read existing coefficient, multiply delay slot
  @(0x0605660C,0x2D02), # overwrite the selected coefficient
  @(0x0605660E,0x4810), # counted loop, 424 entries
  @(0x06056612,0xE04C), # per-entry phase step at +4C
  @(0x06056618,0x3B3C), # phase increment in branch delay slot
  @(0x0605661E,0xE348), # per-frame phase speed at +48
  @(0x06056626,0x0E26), # store next phase at +54
  @(0x0605663A,0x01A8), @(0x0605663C,0x0FFF),
  @(0x06056E56,0x1E0E), # preserve geometric helper result at +38
  @(0x06056E58,0x53E8), # native yaw at +20
  @(0x06056E72,0x4318), @(0x06056E74,0x4308), # <<8 then <<2
  @(0x06056E76,0x633B), @(0x06056E78,0x1E31), # negate, store X at +4
  @(0x06056E7A,0x52EE), @(0x06056E7C,0x9318),
  @(0x06056E7E,0x622B), @(0x06056E80,0x323C),
  @(0x06056E82,0x4228), @(0x06056E84,0x1E22), # (256-offset)<<16 -> +8
  @(0x06056EB0,0x0100)
 )
 foreach($pair in $words) {
  if((Read-World $pair[0] 2) -ne $pair[1]) {
   throw ('WORLD environment instruction differs at {0:X8}' -f $pair[0])
  }
 }
 foreach($pair in @(@(0x0605664C,0x10000),@(0x06056650,0x06035644),@(0x06056654,0x20216E80))) {
  if((Read-World $pair[0] 4) -ne $pair[1]){throw 'WORLD wave literal differs'}
 }
 $op=Read-World 0x06056E86 2
 $disp=$op-band 0xFFF
 if($disp-ge 0x800){$disp-=0x1000}
 if(($op-shr 12)-ne 0xB -or (0x06056E86+4+2*$disp)-ne 0x060565C0){throw 'WORLD wave call differs'}
 Write-Output 'PASS original WORLD coefficient-wave instructions and separate normal-background origins; not a runtime/render test'
} finally {$disc.Dispose()}

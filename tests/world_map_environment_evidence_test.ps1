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
  @(0x060566B8,0x945F), # RAMCTL low-byte PC-relative literal load
  @(0x0605677A,0x00B4), # authored rotation VRAM bank roles (not 0080)
  @(0x060566BE,0x2039), @(0x060566C0,0x240B), # preserve high byte, OR literal
  @(0x060566C4,0x2241), # write pending RAMCTL
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
  @(0x06056EB0,0x0100),
  @(0x06056E94,0x03EE), @(0x06056E98,0x323C), # add environment +40
  @(0x06056E9C,0x1D21), @(0x06056E9E,0x9208),
  @(0x06056EA0,0x3423), @(0x06056EA2,0x8B21), # signed horizon >=196
  @(0x06056EA4,0x6143), @(0x06056EA6,0x4128),
  @(0x06056EAA,0x631B), # negative horizon <<16
  @(0x06056EAE,0x00C4), # signed comparison threshold 196
  @(0x06056EE8,0xD343), @(0x06056EEE,0x1D31),
  @(0x06056EF0,0x2210), # select normal-background layer 0
  @(0x06056EF2,0x55D1), @(0x06056EF6,0x430B), @(0x06056EF8,0x64D2),
  @(0x06056D94,0xDE4A), # use live rotation matrix, not environment camera XYZ
  @(0x06056DAA,0x01DD), @(0x06056DB2,0x05DD),
  @(0x06056DBA,0x4528), @(0x06056DBC,0x53D2), @(0x06056DBE,0x3538),
  @(0x06056DC4,0x54E8), @(0x06056DCE,0x55E7), # matrix row2 Z and Y
  @(0x06056DD2,0x4029), @(0x06056DD6,0x600F), # SHLR16 / EXTS.W
  @(0x06056958,0x02B3), @(0x0605695C,0x013B) # initial amplitude / vertical offset
 )
 foreach($pair in $words) {
  if((Read-World $pair[0] 2) -ne $pair[1]) {
   throw ('WORLD environment instruction differs at {0:X8}' -f $pair[0])
  }
 }
 foreach($pair in @(@(0x0605664C,0x10000),@(0x06056650,0x06035644),@(0x06056654,0x20216E80),
                   @(0x06056FF8,4279500800L),@(0x06057000,0x06028986),
                   @(0x06056978,0x11102A),@(0x0605697C,0x5B059),
                   @(0x06056EC0,0x0605254C),@(0x06056EC8,0x06035664))) {
  if((Read-World $pair[0] 4) -ne $pair[1]){throw 'WORLD wave literal differs'}
 }
 # Resolve the SH-2 PC-relative load itself. Nearby literals are not enough:
 # both camera-origin multiplications load 32.0, not the nearby -90deg angle.
 foreach($pair in @(@(0x06057028,0x10000),@(0x0605703E,0x200000),@(0x06057044,0x200000))) {
  $opcode=Read-World $pair[0] 2
  $literal=(($pair[0]+4)-band -4)+4*($opcode-band 255)
  if(($opcode-shr 12)-ne 0xD -or (Read-World $literal 4)-ne $pair[1]) {
   throw 'WORLD camera-origin PC-relative load differs'
  }
 }
 if((Read-World 0x06056810 4)-ne 0x5B333 -or
    (Read-World 0x06056814 4)-ne 0x1FB33 -or
    (Read-World 0x0605695A 2)-ne 0xFE67 -or
    (Read-World 0x060568D6 2)-ne 0xE2AF) {
  throw 'WORLD environment scale, height or drift initializer differs'
 }
 $op=Read-World 0x06056E86 2
 $disp=$op-band 0xFFF
 if($disp-ge 0x800){$disp-=0x1000}
 if(($op-shr 12)-ne 0xB -or (0x06056E86+4+2*$disp)-ne 0x060565C0){throw 'WORLD wave call differs'}
 Write-Output 'PASS original WORLD coefficient-wave instructions and separate normal-background origins; not a runtime/render test'
} finally {$disc.Dispose()}
$repo=Split-Path $PSScriptRoot -Parent
$boundary=Get-Content -Raw "$repo/src/integration/lagi_world_boundary_vita.cpp"
if($boundary -notmatch 'regs->mE_RAMCTL\s*=\s*\(regs->mE_RAMCTL\s*&\s*0xFF00\)\s*\|\s*0xB4;') {
 throw 'WORLD adapter does not preserve the authored RAMCTL bank roles'
}
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-world-environment-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
& cl /nologo /std:c++20 /EHsc "/I$repo/include" "$PSScriptRoot/world_map_environment_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'WORLD environment fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'WORLD environment fixture failed'}

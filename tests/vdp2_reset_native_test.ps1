param([string]$Build='build-battle-dev',[string]$Disc='D:/Users/Noradninja/Desktop/Panzer Dragoon Saga Disc 1 of 4 (U)(Saturn)/AZEL_1.bin')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$discFile=[IO.File]::OpenRead($Disc)
try {
 function NativeWord([uint32]$address){
  $offset=[long]$address-0x06006000
  $discFile.Position=(58+[long][Math]::Floor($offset/2048))*2352+16+($offset%2048)
  return ($discFile.ReadByte()-shl8)-bor$discFile.ReadByte()
 }
 if((NativeWord 0x06032348)-ne0x9049 -or (NativeWord 0x0603234A)-ne0xC203 -or
    (NativeWord 0x060323DE)-ne0x1300){throw 'Original Saturn VDP2 reset evidence changed'}
} finally {$discFile.Dispose()}
$source=Get-Content -Raw "$repo/$Build/azel_upstream/VDP2.cpp"
$anchor="    memset(pRegisters, 0, offsetof(s_VDP2Regs, m110_CLOFEN)); // LAGI_NATIVE_COLOR_TAIL`n    pRegisters->mE_RAMCTL = 0x1300; // MAIN 060323DE"
if(!$source.Replace("`r`n","`n").Contains($anchor)){throw 'Generated reset lacks native RAMCTL default'}
$bodyStart=$source.IndexOf('void resetVdp2RegsCopies()')
$bodyEnd=$source.IndexOf('void setupVdp2TextLayerColor()',$bodyStart)
$body=$source.Substring($bodyStart,$bodyEnd-$bodyStart)
if(!$body.Contains('resetVdp2RegsCopy(&vdp2Controls.m20_registers[0])') -or
   !$body.Contains('resetVdp2RegsCopy(&vdp2Controls.m20_registers[1])')){throw 'Both native register banks must be reset'}
'PASS original SH2 literal load/store initializes VRSIZE=0 RAMCTL=1300; generated shared reset restores both register banks'

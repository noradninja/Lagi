param([string]$Build='build-battle-dev',[string]$DiscImage='')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$source=Get-Content -Raw (Join-Path $repo "$Build/azel_upstream/battle_battleHud.cpp")
$begin=$source.IndexOf('void battleHud_drawStatusString(')
$end=$source.IndexOf('void s_battleOverlay_20_draw(', $begin)
if($begin -lt 0 -or $end -lt $begin){throw 'Status function boundary missing'}
$body=$source.Substring($begin,$end-$begin)
$match=[regex]::Match($body,'mE_CMDYA = static_cast<s16>\(pThis->m18_part1Y \+ (0x[0-9A-Fa-f]+)\);')
if(!$match.Success){throw 'Status text must add the native HUD slide offset'}
$anchor=[Convert]::ToInt32($match.Groups[1].Value,16)
foreach($slide in @(-24,-12,0,18,36,53,71)) {
 $actual=[int16]($slide+$anchor)
 $original=-(-163-16-$slide+112)
 if($actual -ne $original){throw "Native status position mismatch at slide=$slide"}
 if($actual-(0+$anchor) -ne $slide){throw 'Status label does not move with its panel'}
}
if($DiscImage){
 $disc=[IO.File]::OpenRead($DiscImage)
 try {
  $anchors=@{0x060726BE=0x600B;0x060726C2=0x3028;0x060726C6=0x3018;0x060726C8=0x7070;0x06072724=0x600B;0x060B19AA=0x00A3;0x060AC100=0x0010}
  foreach($address in $anchors.Keys){
   $offset=$address-0x06054000
   $disc.Position=([long]258144+[int][Math]::Floor($offset/2048))*2352+16+($offset%2048)
   $word=($disc.ReadByte()-shl 8)-bor $disc.ReadByte()
   if($word -ne $anchors[$address]){throw ('Original HUD instruction/data mismatch at {0:X8}' -f $address)}
  }
 } finally {$disc.Dispose()}
}
Write-Output 'PASS generated status label shares native panel slide direction and authored anchor; not a hardware animation test.'

param([Parameter(Mandatory=$true)][string]$DiscImage)
$ErrorActionPreference='Stop'
# Original US Disc 1 MAIN, MODE1/2352. Read only; do not extract into the repo.
$main=New-Object byte[] 253952
$stream=[IO.File]::OpenRead($DiscImage)
try {
 for($offset=0;$offset -lt $main.Length;$offset+=2048){
  $null=$stream.Seek((58+[int]($offset/2048))*2352+16,[IO.SeekOrigin]::Begin)
  $count=[Math]::Min(2048,$main.Length-$offset)
  if($stream.Read($main,$offset,$count) -ne $count){throw 'Truncated MAIN extent'}
 }
} finally {$stream.Dispose()}
function Word([int]$ea){$i=$ea-0x06006000;return ([int]$main[$i]*256+[int]$main[$i+1])}
function Words([int]$ea,[string]$expected){
 foreach($hex in ($expected -split ' ')){
  if((Word $ea) -ne [Convert]::ToInt32($hex,16)){throw ('Native entry instruction mismatch at {0:X8}' -f $ea)}
  $ea+=2
 }
}
function Long([int]$ea,[uint32]$expected){
 if(((Word $ea)*65536L+(Word ($ea+2))) -ne $expected){throw ('Native entry literal mismatch at {0:X8}' -f $ea)}
}
# Descriptor establishes init/update/draw/delete identities, not inferred names.
Long 0x06042D50 0x06006814
Long 0x06042D54 0x060068AC
Long 0x06042D58 0x060068D8
Long 0x06042D5C 0x06006956
# Init: current fade -> FFFF, 3C units, repeated for channel +24.
Words 0x0600682A 'DE1B D31B 430B 64E3 6503 D61A 64E3 D21A 420B E73C 64E3 D316 430B 7424 6503 D615 64E3 D215 7424 420B E73C'
Long 0x06006898 0x0604B484
Long 0x0600689C 0x06034C20
Long 0x060068A0 0x0000FFFF
Long 0x060068A4 0x06034AAA
# Complete draw dispatch and case 0: one state increment, then return.
Words 0x060068D8 '2FE6 6E43 4F22 60E1 8800 8903 8801 8908 A032 0009 D233 E301 2230 61E1 7101 A02B 2E11'
# Case 1: overlay index -> COMMON row -> load call. No conditional fade gate
# occurs between case dispatch and this call in the original instruction stream.
Words 0x060068FA 'D231 D131 6422 D531 8541 6303 4008 303C 4008 D32F 041E 430B E600'
Long 0x060069C0 0x06049F4C
Long 0x060069C4 0x002005E0
Long 0x060069C8 0x06054000
Long 0x060069CC 0x0601016C
# Remaining case 1: fonts, overlay entry, state advance and common return.
Words 0x06006914 'D42E D22F 420B 0009 84E2 7001 80E2 D327 D12C 6432 8541 6303 4008 303C 4008 041E 2448 8905 D326 430B 0009 84E2 7001 80E2 D320 430B 64E3 62E1 7201 2E21 4F26 000B 6EF6'
Long 0x060069D4 0x06024FD8
Long 0x060069D8 0x002005E4
Write-Output 'PASS original MAIN battle overlay descriptor, two white-fade requests, complete draw dispatch/load/fonts/entry sequence; no explicit fade wait in this callback. Not loader interrupt or display-timing proof.'

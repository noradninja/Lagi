param([Parameter(Mandatory=$true)][string]$DiscImage)
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$source=Get-Content -Raw "$repo/src/integration/lagi_audio_vita.cpp"
$worker=$source.IndexOf('int audio_worker_thread(')
$load=$source.IndexOf('load_sound_file_list(SoundDataTable.front().m0)', $worker)
$loop=$source.IndexOf('while (!g_audioWorkerStop.load', $worker)
if($load -lt $worker -or $load -ge $loop){throw 'System boot banks not loaded before worker event loop'}
$disc=[IO.File]::OpenRead($DiscImage)
try {
 function Read-Common([long]$ea,[int]$count){
  $out=New-Object byte[] $count
  for($i=0;$i -lt $count;$i++){$offset=$ea+$i-0x200000;$disc.Position=(182+[long][Math]::Floor($offset/2048))*2352+16+($offset%2048);$b=$disc.ReadByte();if($b -lt 0){throw 'Short disc read'};$out[$i]=$b};return ,$out
 }
 function BE([byte[]]$bytes){$v=0L;foreach($b in $bytes){$v=($v-shl 8)-bor $b};$v}
 $table=BE (Read-Common 0x2152D8 4)
 $expected=@(@('SYS_SE.SEQ',0x25A0B000L,340),@('SYS_SE.BIN',0x25A0B300L,7192))
 for($i=0;$i -lt 2;$i++) {
  $entry=$table+$i*12;$nameEA=BE (Read-Common $entry 4)
  $name=([Text.Encoding]::ASCII.GetString((Read-Common $nameEA 16))).Split([char]0)[0]
  $dest=BE (Read-Common ($entry+4) 4);$mask=BE (Read-Common ($entry+8) 4)
  if($name -ne $expected[$i][0] -or $dest -ne $expected[$i][1] -or $mask -ne 1){throw 'Native persistent system SE descriptor differs'}
 }
 $title=BE (Read-Common (0x2152D8+75*16) 4)
 if($title -eq $table){throw 'Title and bootstrap lists unexpectedly equal'}
 'PASS native COMMON bootstrap includes global system sequence/sample banks; title uses a different list; production startup restores bootstrap before events'
} finally {$disc.Dispose()}

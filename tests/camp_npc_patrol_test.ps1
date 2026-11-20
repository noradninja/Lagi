param([Parameter(Mandatory=$true)][string]$Build,[Parameter(Mandatory=$true)][string]$DiscImage)
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$disc=[IO.File]::OpenRead($DiscImage)
try {
 function Read-Camp([long]$ea,[int]$count){$v=0L;for($i=0;$i-lt$count;$i++){$o=$ea+$i-0x6054000;$disc.Position=(239428+[long][Math]::Floor($o/2048))*2352+16+($o%2048);$b=$disc.ReadByte();if($b-lt0){throw 'Short CAMP read'};$v=($v-shl8)-bor$b};return $v}
 foreach($p in @(@(0x60709F6,0x3C33),@(0x6070A14,0x8D09),@(0x6070A40,0x1B01),
  @(0x6070A78,0x2B00),@(0x6070ADE,0x00EE),@(0x6070AEC,0x6122),
  @(0x6070B10,0x4315),@(0x6070BE8,0x3233),@(0x6070BFA,0xC90F),
  @(0x6070C5A,0xE48F),@(0x6070C82,0x932A),@(0x6070C96,0x1C01),
  @(0x6070DD6,0x1C21),@(0x6070E08,0x1E39),@(0x6070E68,0x1C91),
  @(0x606F444,0x0E24),@(0x606F578,0x8D03),@(0x606FF70,0xA008),
  @(0x60717B6,0x70FF),@(0x60717BA,0x7001))) {
  if((Read-Camp $p[0] 2)-ne$p[1]){throw ('Camp NPC instruction differs at{0:X8}'-f$p[0])}
 }
 foreach($p in @(@(0x6080148,0x606F3F0),@(0x608014C,0x606F530),@(0x6080150,0),@(0x6080154,0x606FF04),
  @(0x606FFA4,0x606F8BA),@(0x606FFA8,0x606F974),@(0x606FFAC,0x6070ABC),@(0x606FFB0,0x606F8D8),
  @(0x607181C,0x6080180),@(0x6070AB0,0x60266B0),@(0x6070CE4,0x6035664),
  @(0x6070EA4,0xAAAAAAA),@(0x6070EAC,0x5555555))) {
  if((Read-Camp $p[0] 4)-ne$p[1]){throw ('Camp NPC binding differs at{0:X8}'-f$p[0])}
 }
}finally{$disc.Dispose()}
$generated=Get-Content -Raw "$repo/$Build/azel_upstream/town_camp_twn_camp.cpp"
foreach($pattern in @('case 0x06080148:','return lagiCreateCampNPC\(parent, arg\)',
 'lagiCampNpcInit\(pThis, arg\)','lagiCreateCampNpcBody\(pThis\)',
 'getSaturnPtr\(0x06080180\)','register_work_area_disposer\(task',
 'begin_view_relative_submission_scope','end_view_relative_submission_scope')) {
 if($generated-notmatch$pattern){throw "Camp NPC generated integration absent: $pattern"}
}
$actor=[regex]::Match($generated,'(?s)struct sLagiCampNPC :.*?// All fields in sNPC base').Value
if(!$actor-or$actor-match'Unimplemented|initEdgeNPC\('){throw 'Camp actor still has a proxy/stub initializer'}
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-camp-patrol-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
& cl /nologo /std:c++20 /EHsc "/I$repo/include" "$PSScriptRoot/camp_npc_patrol_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE){throw 'Camp patrol fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE){throw 'Camp patrol fixture failed'}
# Compile the actual generated resource/lifecycle methods, with native matrix,
# model and task services supplied by the fixture. Active animation/patrol math
# is validated separately above; this checks their concrete receiver wiring.
function Get-ProductionFunction([string]$text,[string]$signature){
 $start=$text.IndexOf($signature);if($start-lt0){throw "Production method absent: $signature"}
 $open=$text.IndexOf('{',$start);$depth=0
 for($i=$open;$i-lt$text.Length;$i++){
  if($text[$i]-eq'{'){$depth++};if($text[$i]-eq'}'){$depth--;if(!$depth){return $text.Substring($start,$i-$start+1)}}
 };throw 'Unbalanced production method'
}
$controls=Get-Content -Raw "$repo/src/integration/lagi_camp_npc_controls.inc"
[IO.File]::WriteAllText("$scratch/production_init.inc",(Get-ProductionFunction $controls 'static void lagiCampNpcInit(sNPC* npc,sSaturnPtr arg)'))
$methods=''
foreach($signature in @('static void Init(sLagiCampNPC* pThis, sSaturnPtr arg)',
 'static void Update(sLagiCampNPC* pThis)','static void Draw2(sLagiCampNPC* pThis)',
 'static void Delete(sLagiCampNPC* pThis)')) {
 $methods+=(Get-ProductionFunction $generated $signature)+"`n"
}
[IO.File]::WriteAllText("$scratch/production_methods.inc",$methods)
& cl /nologo /std:c++20 /EHsc "/I$scratch" "$PSScriptRoot/camp_npc_actor_test.cpp" "/Fe:$scratch/actor.exe" "/Fo:$scratch/actor.obj"
if($LASTEXITCODE){throw 'Camp actor fixture compile failed'}
& "$scratch/actor.exe"
if($LASTEXITCODE){throw 'Camp actor fixture failed'}

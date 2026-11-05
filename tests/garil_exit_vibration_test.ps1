param([Parameter(Mandatory=$true)][string]$DiscImage,[string]$Build='build/world-garil')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$disc=[IO.File]::OpenRead($DiscImage)
try {
 function Read-A5([long]$ea,[int]$count){$v=0L;for($i=0;$i-lt$count;$i++){$o=$ea+$i-0x6054000;$disc.Position=(244678+[long][Math]::Floor($o/2048))*2352+16+($o%2048);$b=$disc.ReadByte();if($b-lt0){throw 'Short A5 read'};$v=($v-shl8)-bor$b};return $v}
 foreach($pair in @(@(0x6056710,0x52EB),@(0x6056714,0x12B4),@(0x6056728,0x51EB),
  @(0x605672E,0x1124),@(0x60561B6,0x50E3),@(0x60561BC,0x50E5),
  @(0x60561F4,0x50E5),@(0x60561F8,0xC903),@(0x6056200,0x1E05),
  @(0x6056202,0x53E4),@(0x6056204,0x1E33))) {
  if((Read-A5 $pair[0] 2)-ne$pair[1]){throw 'Native Garil stored/current intensity binding differs'}
 }
 if((Read-A5 0x60567BC 4)-ne0x1B4E81){throw 'Native vibration intensity differs'}
}finally{$disc.Dispose()}
$generated=Get-Content -Raw "$repo/$Build/azel_upstream/field_field_a5_a5_exitTriggerEntity.cpp"
if($generated.Contains('*(s32*)((u8*)pThis->m2C_childTask + 0x10)')){throw 'Raw native child offset remains'}
$prefix='static_cast<sA5ExitTriggerVibrationTask*>(pThis->m2C_childTask)->m14_storedIntensity = '
$set=$prefix+'0x1B4E81;';$clear=$prefix+'0;'
if(!$generated.Contains($set)-or!$generated.Contains($clear)){throw 'Typed stored-intensity writes missing'}
$structure=[regex]::Match($generated,'(?s)struct sA5ExitTriggerVibrationTask :.*?\n};').Value
$start=$generated.IndexOf('static void a5ExitTriggerVibration_Update(')
$end=$generated.IndexOf('// 0605620c',$start)
if(!$structure-or$start-lt0-or$end-le$start){throw 'Native vibration child boundary changed'}
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-garil-vibration-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
[IO.File]::WriteAllText("$scratch/production.inc",$structure+"`n"+$generated.Substring($start,$end-$start))
[IO.File]::WriteAllText("$scratch/set.inc",$set)
[IO.File]::WriteAllText("$scratch/clear.inc",$clear)
& cl /nologo /std:c++20 /EHsc "/I$scratch" "$PSScriptRoot/garil_exit_vibration_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE){throw 'Garil vibration fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE){throw 'Garil vibration fixture failed'}

param([Parameter(Mandatory)][string]$Disc,[switch]$ShowHorizontal)
$ErrorActionPreference='Stop'
# Read-only MODE1/2352 audit. No game bytes are written to the repository.
$stream=[IO.File]::OpenRead($Disc)
function Extent([long]$lba,[int]$size){
 if($size -lt 0 -or $size -gt 16777216){throw 'Unexpected disc extent'}
 $bytes=New-Object byte[] $size
 for($i=0;$i -lt $size;$i+=2048){
  $null=$stream.Seek(($lba+[long][Math]::Floor($i/2048))*2352+16,0)
  $want=[Math]::Min(2048,$size-$i)
  if($stream.Read($bytes,$i,$want) -ne $want){throw 'Short disc read'}
 }
 return ,$bytes
}
function BE($bytes,[int]$at,[int]$count=4){
 if($at -lt 0 -or $at+$count -gt $bytes.Length){throw 'Resource bounds'}
 [long]$v=0;for($j=0;$j -lt $count;$j++){$v=$v*256+$bytes[$at+$j]};return $v
}
try{
 $pvd=Extent 16 2048
 if([Text.Encoding]::ASCII.GetString($pvd,1,5) -ne 'CD001'){throw 'Expected MODE1/2352 ISO9660'}
 $root=Extent ([BitConverter]::ToInt32($pvd,158)) ([BitConverter]::ToInt32($pvd,166))
 $files=@{}
 for($i=0;$i -lt $root.Length;){
  $len=$root[$i];if(!$len){$i=([int][Math]::Floor($i/2048)+1)*2048;continue}
  if($len -lt 34 -or $i+$len -gt $root.Length -or 33+$root[$i+32] -gt $len){throw 'Malformed ISO directory'}
  $name=[Text.Encoding]::ASCII.GetString($root,$i+33,$root[$i+32]).Split(';')[0]
  $files[$name]=@([BitConverter]::ToInt32($root,$i+2),[BitConverter]::ToInt32($root,$i+10));$i+=$len
 }
 $prog=Extent $files['FLD_A3.PRG'][0] $files['FLD_A3.PRG'][1]
 $tables=@(0x6085AA4,0x608838C,0x608BE04)
 for($sub=0;$sub -lt 3;$sub++){
  $mcb=Extent $files["FLD_A3_$sub.MCB"][0] $files["FLD_A3_$sub.MCB"][1]
  $at=$tables[$sub]-0x6054000;$grid=(BE $prog $at)-0x6054000
  $cells=(BE $prog ($at+16))*(BE $prog ($at+20))
  if($cells -gt 4096){throw 'Unexpected grid size'}
  $visual=[Collections.Generic.HashSet[int]]::new();$collision=[Collections.Generic.HashSet[int]]::new()
  for($cell=0;$cell -lt $cells;$cell++){
   $ptr=BE $prog ($grid+$cell*4);if(!$ptr){continue};$entry=$ptr-0x6054000
   for($n=0;$n -lt 4096;$n++){
    $modelArray=BE $prog ($entry+$n*24);if(!$modelArray){break};$modelArray-=0x6054000
    for($lod=0;$lod -lt 5;$lod++){
     $off=BE $prog ($modelArray+$lod*2) 2
     if($off){if($lod -eq 4){$null=$collision.Add($off)}else{$null=$visual.Add($off)}}
    }
   }
   if($n -eq 4096){throw 'Missing grid sentinel'}
  }
  "sub=$sub visualModels=$($visual.Count) collisionModels=$($collision.Count) grid3=$('{0:X8}' -f (BE $prog ($at+8)))"
  $flags=@{};$flat=0;$quads=0
  foreach($off in $visual){
   $model=BE $mcb $off;if(!$model){continue}
   $verts=BE $mcb ($model+4);$vertexAt=BE $mcb ($model+8);$q=$model+12
   if($verts -gt 65535){throw 'Unexpected vertex count'}
   for($n=0;$n -lt 65536;$n++){
    $indices=@(0,2,4,6|ForEach-Object{BE $mcb ($q+$_) 2})
    if(($indices|Measure-Object -Sum).Sum -eq 0){break}
    foreach($index in $indices){if($index -ge $verts){throw 'Invalid model index'}}
    $light=BE $mcb ($q+8) 2;$ctrl=BE $mcb ($q+10) 2;$pmod=BE $mcb ($q+12) 2;$colr=BE $mcb ($q+14) 2
    $key='{0:X4}/{1:X4}' -f $ctrl,$pmod
    $flags[$key]=1+$flags[$key];$quads++
    $ys=@($indices|ForEach-Object{BE $mcb ($vertexAt+$_*6+2) 2}|Select-Object -Unique)
    if($ys.Count -eq 1){$flat++;if($ShowHorizontal){' horizontal modelSlot={0:X4} quad={1} ctrl={2:X4} pmod={3:X4} colr={4:X4} yRaw={5:X4}' -f $off,$n,$ctrl,$pmod,$colr,$ys[0]}}
    $extra=@(0,8,48,24)[($light -shr 8)-band 3];$q+=20+$extra
   }
   if($n -eq 65536){throw 'Missing model sentinel'}
  }
  "quads=$quads horizontal=$flat flags="+(($flags.GetEnumerator()|Sort-Object Name|ForEach-Object{"$($_.Name):$($_.Value)"})-join ' ')
 }
}finally{$stream.Dispose()}

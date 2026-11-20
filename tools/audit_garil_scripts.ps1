param([Parameter(Mandatory=$true)][string]$DiscImage,[switch]$AsObject)
$ErrorActionPreference='Stop'
# Read-only structural bytecode traversal, not script execution or a VM fallback.
$disc=[IO.File]::OpenRead((Resolve-Path -LiteralPath $DiscImage).Path)
try {
 $base=0x06054000L
 $image=New-Object byte[] 316576
 for($offset=0;$offset-lt$image.Length;$offset+=2048){
  $disc.Position=(244678+[long][Math]::Floor($offset/2048))*2352+16
  $count=[Math]::Min(2048,$image.Length-$offset)
  if($disc.Read($image,$offset,$count)-ne$count){throw 'Short FLD_A5 image'}
 }
 function Read-A5([long]$ea,[int]$count){
  if($ea-lt$base-or$ea+$count-gt$base+$image.Length){throw ('A5 address outside image: {0:X8}'-f$ea)}
  $value=0L;for($i=0;$i-lt$count;$i++){$value=($value-shl8)-bor$image[$ea-$base+$i]};return $value
 }
 if((Read-A5 0x06054170 4)-ne0x0609D4A8){throw 'Original A5 script table differs'}
 $pending=[Collections.Generic.Queue[long]]::new()
 $roots=[Collections.Generic.List[long]]::new()
 $metadata=[Collections.Generic.List[object]]::new()
 for($entry=0x0609D4A8L;;$entry+=4){
  $target=Read-A5 $entry 4;if(!$target){break}
  $index=$roots.Count
  $roots.Add($target)
  if($index-in0x16,0x17,0x18){
   # Native subfields pass these entries to 0607A9A0 -> 06070D94,
   # the encounter-list consumer, not the script interpreter.
   $record=$target;$records=0
   while((Read-A5 $record 1)-ne1){
    if((Read-A5 $record 1)-eq0x7F){
     $record=(($record+4)-band-4)+0x14;$records++
    }else{$record++}
   }
   $metadata.Add([pscustomobject]@{Index=$index;EA=$target;Records=$records})
  }else{$pending.Enqueue($target)}
  if($roots.Count-gt256){throw 'Unterminated A5 script table'}
 }
 $visited=[Collections.Generic.HashSet[long]]::new()
 $calls=[Collections.Generic.List[object]]::new()
 $unknown=[Collections.Generic.List[object]]::new()
 :walk while($pending.Count){
  $pc=$pending.Dequeue();if(!$visited.Add($pc)){continue}
  if($visited.Count-gt100000){throw 'A5 script traversal limit'}
  $opcode=Read-A5 $pc 1;$next=$pc+1
  switch($opcode){
   1 {continue walk} # END; call continuations are independently queued below.
   {$_-in0,22,24,25,26,36} {}
   {$_-in2,4,7} {$next=(($next+1)-band-2)+2}
   {$_-in10,11,12,39} {
    $operand=($next+3)-band-4
    $pending.Enqueue((Read-A5 $operand 4))
    if($opcode-eq10){continue walk}
    $next=$operand+4
   }
   {$_-in14,38} {
    $arguments=Read-A5 $next 1;$operand=($next+4)-band-4
    $calls.Add([pscustomobject]@{ScriptEA=$pc;Opcode=$opcode;Arguments=$arguments;NativeEA=(Read-A5 $operand 4)})
    if($arguments-gt1){$unknown.Add([pscustomobject]@{EA=$pc;Opcode=$opcode;Reason='native argument count exceeds recovered VM'});continue walk}
    $next=$operand+4*($arguments+1)
   }
   {$_-in21,34} {$next=(($next+3)-band-4)+4}
   27 {$next=((($next+1)-band-2)+5)-band-4;$next+=4}
   {$_-in30,31,33} {
    $choices=Read-A5 $next 1;if($choices-ge128){$choices-=256}
    $next++
    if($choices-gt0){$next=(($next+3)-band-4)+4*$choices}
   }
   default {$unknown.Add([pscustomobject]@{EA=$pc;Opcode=$opcode;Reason='opcode not implemented by recovered shared VM'});continue walk}
  }
  $pending.Enqueue($next)
 }
 $result=[pscustomobject]@{Roots=$roots.ToArray();Metadata=$metadata.ToArray();Visited=$visited.Count;Calls=$calls.ToArray();Unknown=$unknown.ToArray()}
 if($AsObject){return $result}
 "A5 structural audit: tableEntries=$($roots.Count) metadata=$($metadata.Count) instructions=$($visited.Count) nativeCalls=$($calls.Count) unsupported=$($unknown.Count); not execution evidence"
 $calls | Group-Object NativeEA,Arguments | ForEach-Object {
  $first=$_.Group[0];'{0:X8} args={1} sites={2}'-f$first.NativeEA,$first.Arguments,$_.Count
 }
 foreach($item in $unknown){'{0:X8}: opcode={1} {2}'-f$item.EA,$item.Opcode,$item.Reason}
} finally {$disc.Dispose()}

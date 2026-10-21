param([Parameter(Mandatory=$true)][string]$Build)
$ErrorActionPreference='Stop'
$path=Join-Path $Build 'build.ninja'
$lines=[IO.File]::ReadAllLines((Resolve-Path -LiteralPath $path).Path)
$targets=[ordered]@{
 Neptune='src/platform/vita/neptune_renderer.cpp.obj'
 Runtime='src/integration/lagi_runtime.cpp.obj'
 TaskHost='src/integration/lagi_task_vita.cpp.obj'
 RenderBridge='src/integration/lagi_render_bridge.cpp.obj'
 Native3D='azel_upstream/3dEngine.cpp.obj'
 Animation='azel_upstream/kernel_animation.cpp.obj'
 BattleEngine='azel_upstream/battle_battleEngine.cpp.obj'
 BroodQueen='azel_upstream/battle_BTL_A3_baldorQueen.cpp.obj'
 Urchin='azel_upstream/battle_BTL_A3_urchin.cpp.obj'
}
foreach($name in $targets.Keys){
 $flags='';$defines='';$found=$false
 for($i=0;$i -lt $lines.Length;++$i){
  if(!$lines[$i].StartsWith('build ')){continue}
  $object=($lines[$i] -split ': ',2)[0].Replace('\','/')
  if(!$object.EndsWith('/'+$targets[$name])){continue}
  $found=$true
  for($j=$i+1;$j -lt $lines.Length -and !$lines[$j].StartsWith('build ');++$j){
   if($lines[$j] -match '^  FLAGS = (.*)$'){$flags=$Matches[1]}
   if($lines[$j] -match '^  DEFINES = (.*)$'){$defines=$Matches[1]}
  }
  break
 }
 $opts=[regex]::Matches($flags,'(?:^|\s)(-O(?:[0-3sgz]|fast))(?=\s|$)')
 $optimization='compiler default (no -O flag)'
 if($opts.Count){$optimization=$opts[$opts.Count-1].Groups[1].Value}
 $math=[regex]::Matches($flags,'(?:^|\s)(-f(?:no-)?fast-math)(?=\s|$)')
 $fastMath=$false
 if($math.Count){$fastMath=$math[$math.Count-1].Groups[1].Value -eq '-ffast-math'}
 [pscustomobject]@{
  Source=$name;Found=$found;Optimization=$optimization
  NDebug=([regex]::IsMatch("$defines $flags",'(?:^|\s)-DNDEBUG(?:=\S+)?(?=\s|$)'))
  ExplicitFastMath=$fastMath;Flags=$flags
 }
}

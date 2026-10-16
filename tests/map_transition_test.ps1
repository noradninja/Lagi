$ErrorActionPreference='Stop'
$root=(Resolve-Path "$PSScriptRoot/..").Path
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-map-transition-'+[guid]::NewGuid())
New-Item -ItemType Directory -Path $scratch | Out-Null
$production=Get-Content -Raw "$root/src/integration/lagi_map_transition_vita.cpp"
$production=[regex]::Replace($production,'(?m)^#include[^\r\n]*','')
$fixture=@'
#include <cassert>
#include <vector>
struct LagiAzelMapConfirmationState {bool ready=false,finished=false,running=false; int result=0;};
struct Channel {int value=77;};
struct Fades {Channel m0_fade0,m24_fade1;};
struct Backup {Fades m2B0;};
struct Engine {Backup* m40E4=nullptr; struct {int m2=0;} m40AC;} graphicEngineStatus;
Fades liveFades;
std::vector<int> calls;
Channel* targets[2];
int targetCount=0;
void setNextGameStatus(int status) {assert(status==0x4F);calls.push_back(1);}
void fadePalette(Channel* channel,int from,int to,int frames) {
 assert(from==0 && to==0 && frames==1);
 assert(channel!=&liveFades.m0_fade0 && channel!=&liveFades.m24_fade1);
 targets[targetCount++]=channel; channel->value=0;calls.push_back(2);
}
'@
$checks=@'
int main() {
 Backup backup;
 graphicEngineStatus.m40E4=&backup;
 for(int finished=0;finished<2;++finished) for(int running=0;running<2;++running)
 for(int result=0;result<2;++result) {
  calls.clear();targetCount=0;backup={};graphicEngineStatus.m40AC.m2=0;
  LagiAzelMapConfirmationState state{true,bool(finished),bool(running),result};
  const bool expected=finished && !running && result;
  assert(lagiAzelCommitMapWorldConfirmation(state)==expected);
  assert(graphicEngineStatus.m40AC.m2==int(expected));
  if(expected) {
   assert((calls==std::vector<int>{1,2,2}));
   assert(targets[0]==&backup.m2B0.m0_fade0 && targets[1]==&backup.m2B0.m24_fade1);
  } else {assert(calls.empty());assert(backup.m2B0.m0_fade0.value==77);}
 }
 calls.clear();targetCount=0;graphicEngineStatus.m40E4=nullptr;graphicEngineStatus.m40AC.m2=0;
 assert(lagiAzelCommitMapWorldConfirmation({true,true,false,1}));
 assert((calls==std::vector<int>{1}) && graphicEngineStatus.m40AC.m2==1);
 assert(liveFades.m0_fade0.value==77 && liveFades.m24_fade1.value==77);
}
'@
[IO.File]::WriteAllText("$scratch/test.cpp",$fixture+"`n"+$production+"`n"+$checks)
& cl /nologo /std:c++20 /EHsc "$scratch/test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Transition fixture compilation failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Transition assertions failed'}
Write-Output 'PASS: production Map transition confirmation guards, call order, saved fade targets and null backup; no hardware validation.'

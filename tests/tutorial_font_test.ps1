param([string]$BuildDirectory='build-battle-dev',[string]$ScratchRoot='../tutorial-font-test')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $repo $ScratchRoot))
New-Item -ItemType Directory -Force $scratch | Out-Null
$source=Get-Content -Raw "$repo/$BuildDirectory/azel_upstream/battle_BTL_T0_BTL_T0.cpp"
$begin=$source.IndexOf('static void tutorialController_setFont(char param_1)')
$end=$source.IndexOf('// 0605a61a',$begin)
if($begin-lt 0 -or $end-le $begin){throw 'Generated tutorial font boundary changed'}
$function=$source.Substring($begin,$end-$begin)
$fixture=@'
#include <cassert>
#include <cstdint>
struct Text {unsigned char hostTaskHeader[128]{};std::int8_t m17=0,m18=0;}text;
using sBattleTextDisplayTask=Text;
struct Overlay {Text* m14_textDisplay=&text;}overlay;
struct Manager {Overlay* m10_battleOverlay=&overlay;}manager;
Manager* gBattleManager=&manager;
int selected=999;
void setActiveFont(int value){selected=value;}
'@
$checks=@'
int main(){
 for(int a=-128;a<128;++a)for(int b=-128;b<128;++b){
  text.m17=a;text.m18=b;
  tutorialController_setFont(0);assert(selected==b);
  tutorialController_setFont(1);assert(selected==a);
  tutorialController_setFont(-1);assert(selected==a);
 }
}
'@
[IO.File]::WriteAllText("$scratch/test.cpp",$fixture+"`n"+$function+"`n"+$checks)
& cl /nologo /std:c++20 /EHsc "$scratch/test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE-ne 0){throw 'Tutorial font compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE-ne 0){throw 'Tutorial font assertions failed'}
Write-Output 'PASS: generated tutorial font member selection across 65536 signed font pairs and padded host task layout; no device validation'

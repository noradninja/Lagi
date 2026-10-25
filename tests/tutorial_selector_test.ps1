param([string]$ScratchRoot='../tutorial-selector-test')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $repo $ScratchRoot))
New-Item -ItemType Directory -Force $scratch | Out-Null
$source=Get-Content -Raw "$repo/extern/Azel/AzelLib/field/field_a3/o_fld_a3.cpp"
$start=$source.IndexOf('s32 dispatchTutorialMultiChoice()')
$end=$source.IndexOf('return result;',$start)
if($start-lt 0 -or $end-le $start){throw 'Tutorial selector boundary changed'}
$function=$source.Substring($start,$end-$start)+'return result;'+"`n}"
$fixture=@'
#include <array>
#include <vector>
#include <cassert>
using s32=int;using s8=signed char;
struct Choice {int m4_currentChoice=0,mC=99;}choice;
struct Script {Choice* m44_multiChoiceData=&choice;}script;
struct Sub {Script* m34C_ptrToE=&script;}sub;
struct Field {int m2C_currentFieldIndex=2;Sub* m8_pSubFieldData=&sub;}field;
Field* getFieldTaskPtr(){return &field;}
std::vector<int> calls;
struct State {void setBit(int bit){assert(bit==0xA2*8+2);calls.push_back(1);}}mainGameState;
void dispatchTutorialMultiChoiceSub1(){calls.push_back(2);}
void fadeOutAllSequences(){calls.push_back(3);}
void dispatchTutorialMultiChoiceSub2(){calls.push_back(4);}
void dispatchTutorialMultiChoiceSub3(Script* p,int result){assert(p==&script && result>=5 && result<=7);calls.push_back(100+result);}
void playSystemSoundEffect(int sound){assert(sound==3);calls.push_back(5);}
void startBattleTutorial(int index,int param){assert(param==-1 && index>=0xB && index<=0xF);calls.push_back(index);}
void markMultiChoiceEntrySeen(int index){calls.push_back(200+index);}
'@
$checks=@'
int main(){
 for(int fieldIndex:{1,2,21})for(int selected=0;selected<=8;++selected){
  field.m2C_currentFieldIndex=fieldIndex;choice={selected,99};calls.clear();
  assert(dispatchTutorialMultiChoice()==selected);
  if(selected==8){assert(choice.mC==99);assert((calls==(fieldIndex==2?std::vector<int>{1,2}:std::vector<int>{3,4})));}
  else if(selected<5){assert(choice.mC==1);assert((calls==std::vector<int>{0xB+selected,200+selected}));}
  else {assert(choice.mC==0);assert((calls==std::vector<int>{100+selected,5,200+selected}));}
 }
}
'@
[IO.File]::WriteAllText("$scratch/test.cpp",$fixture+"`n"+$function+"`n"+$checks)
& cl /nologo /std:c++20 /EHsc "$scratch/test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE-ne 0){throw 'Tutorial selector compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE-ne 0){throw 'Tutorial selector assertions failed'}
Write-Output 'PASS native A3 tutorial selector: all nine choices, five battle lessons, three text lessons, field/title exit distinction and seen-entry order; host service mocks only'

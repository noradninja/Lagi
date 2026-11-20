param([string]$ScratchRoot='../tutorial-a7-selector-test')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $repo $ScratchRoot))
New-Item -ItemType Directory -Force $scratch | Out-Null
$source=Get-Content -Raw "$repo/extern/Azel/AzelLib/field/field_a7/o_fld_a7.cpp"
$start=$source.IndexOf('static s32 getMultiChoiceResult_A7()')
$end=$source.IndexOf('s32 FLD_A7_data::executeNative', $start)
if($start-lt 0 -or $end-le $start){throw 'Native A7 selector boundaries changed'}
$functions=$source.Substring($start,$end-$start)
$fixture=@'
#include <array>
#include <vector>
#include <cassert>
#include <cstdio>
using s32=int;using p_workArea=void*;
struct Choice {int m4_currentChoice=0,mC=99;std::array<int,6> m0_choiceTable{1,1,1,1,1,1};}choice;
using s_multiChoice=Choice;
struct Script {Choice* m44_multiChoiceData=&choice;}script;
struct Sub {Script* m34C_ptrToE=&script;}sub;
struct Field {int m2C_currentFieldIndex=22;Sub* m8_pSubFieldData=&sub;}field;
Field* getFieldTaskPtr(){return &field;}
struct State {std::array<unsigned char,256>bitField{};}mainGameState;
std::vector<int> calls;
void releaseMultiChoiceBlocks(){calls.push_back(1);}
void fadeOutAllSequences(){calls.push_back(2);}
void dispatchTutorialMultiChoiceSub2(){calls.push_back(3);}
void a7DialogChoice_spawn_0605ea52(void* parent,int choiceIndex){assert(parent==&script);calls.push_back(100+choiceIndex);}
void playSystemSoundEffect(int sound){assert(sound==3);calls.push_back(4);}
void markMultiChoiceEntrySeen(int selected){calls.push_back(200+selected);}
'@
$checks=@'
int main(){
 for(int fieldIndex:{4,22})for(int selected=0;selected<6;++selected){
  field.m2C_currentFieldIndex=fieldIndex;choice={};choice.m4_currentChoice=selected;calls.clear();mainGameState.bitField.fill(0x81);
  assert(getMultiChoiceResult_A7()==selected);
  for(unsigned i=0;i<6;++i)assert(choice.m0_choiceTable[i]==((selected==4&&i==4)?0:1));
  assert(handleMultiChoiceResult_A7()==selected);
  if(selected==5){assert(choice.mC==99);assert((calls==(fieldIndex==4?std::vector<int>{1}:std::vector<int>{2,3})));}
  else {assert(choice.mC==0);assert((calls==std::vector<int>{100+selected,4,200+selected}));}
  for(unsigned i=0;i<256;++i)assert(mainGameState.bitField[i]==((i==0xa3&&selected==5&&fieldIndex==4)?0x91:0x81));
 }
 puts("PASS production A7 selector: five native topics, choice-4 refresh, sound/seen order and story/title exit distinction; service mocks, no device acceptance");
}
'@
[IO.File]::WriteAllText("$scratch/test.cpp",$fixture+"`n"+$functions+"`n"+$checks)
& cl /nologo /std:c++20 /EHsc "$scratch/test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE-ne 0){throw 'A7 tutorial selector compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE-ne 0){throw 'A7 tutorial selector assertions failed'}

param([string]$BuildDirectory='build-battle-dev')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$cases=@(
 @('field/fieldCutsceneTask.cpp','field_fieldCutsceneTask.cpp','void s_cutsceneTask::Init','void cutsceneTaskUpdateSub0'),
 @('field/fieldCutsceneTask2.cpp','field_fieldCutsceneTask2.cpp','void s_cutsceneTask2::UpdateSub1','void s_cutsceneTask2::Update('),
 @('field/field_a3/o_fld_a3.cpp','field_field_a3_o_fld_a3.cpp','void scriptFunction_6067E68_fadeOut','void scriptFunction_6067ec0_fadeIn')
)
function Slice([string]$text,[string]$begin,[string]$end){
 $a=$text.IndexOf($begin);$b=$text.IndexOf($end,$a)
 if($a -lt 0 -or $b -le $a){throw "Timeline source boundary changed: $begin"}
 return $text.Substring($a,$b-$a).Replace("`r`n","`n").Trim()
}
foreach($case in $cases){
 $native=Slice (Get-Content -Raw "$repo/extern/Azel/AzelLib/$($case[0])") $case[2] $case[3]
 $generated=Slice (Get-Content -Raw "$repo/$BuildDirectory/azel_upstream/$($case[1])") $case[2] $case[3]
 if(!$generated.Contains('static unsigned timelineBudget=16;') -or !$generated.Contains('[FieldExitTimeline]')){throw 'Bounded timeline observer missing'}
 $stripped=($generated -split "`n" | Where-Object {$_ -notmatch 'timelineBudget'}) -join "`n"
 if($stripped.Trim() -cne $native){throw "Timeline observer changed simulation body: $($case[0])"}
}
'PASS all three generated timeline observers are bounded additions; removing observer lines reproduces original function bodies exactly'

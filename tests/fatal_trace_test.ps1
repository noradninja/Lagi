param([string]$ScratchRoot='../fatal-trace-test',[string]$BattleBuild='build-battle-dev')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $repo $ScratchRoot))
New-Item -ItemType Directory -Force $scratch | Out-Null
$source=Get-Content -Raw "$repo/src/platform/vita/vita_fatal_trace.cpp"
# Replace only platform headers and the compiler return-address intrinsic.
# Run the complete production reporting/wrapper bodies with test IO/unwind.
$source=[regex]::Replace($source,'(?m)^#include <(?:psp2/[^>]+|unwind.h)>\r?\n','')
$source=$source.Replace('__builtin_return_address(0)','testCaller()')
[IO.File]::WriteAllText("$scratch/fatal_trace.inc",$source)
$generated=Get-Content -Raw "$repo/$BattleBuild/azel_upstream/town_e011_twn_e011.cpp"
foreach($anchor in @('lagiSetE011FatalStage("    initDramAllocator','lagiSetE011FatalStage("    startScriptTask','lagiSetE011FatalStage("    startE011BackgroundTask','lagiSetE011FatalStage("    startCameraTask')){
 if(!$generated.Contains($anchor)){throw "Missing generated E011 stage: $anchor"}
}
# /EHs lets the test doubles model noreturn C entry points with a sentinel throw.
& cl /nologo /O2 /std:c++20 /EHs "/I$scratch" "$PSScriptRoot/fatal_trace_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Fatal trace fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Fatal trace fixture failed'}

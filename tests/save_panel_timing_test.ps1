param([string]$AzelSourceRoot = 'E:/dev/Lagi/extern/Azel/AzelLib')
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path "$PSScriptRoot/..").Path
$scratch = Join-Path ([System.IO.Path]::GetTempPath()) ('lagi-save-panel-' + [guid]::NewGuid())
New-Item -ItemType Directory -Path $scratch | Out-Null
# Evaluate the production transformation, rather than recreating its patch.
$cmake = (Get-Content -Raw "$root/CMakeLists.txt").Replace("`r`n", "`n")
$begin = $cmake.IndexOf('function(lagi_prepare_upstream_azel_source ')
$end = $cmake.IndexOf('endfunction()', $begin) + 'endfunction()'.Length
$adapter = $cmake.Substring($begin, $end - $begin)
$script = @"
cmake_minimum_required(VERSION 3.18)
set(LAGI_AZEL_SOURCE_ROOT "$AzelSourceRoot")
set(LAGI_AZEL_GENERATED_ROOT "$($scratch.Replace('\','/'))/generated")
file(MAKE_DIRECTORY "`${LAGI_AZEL_GENERATED_ROOT}")
function(set_source_files_properties)
endfunction()
$adapter
lagi_prepare_upstream_azel_source("kernel/loadSavegameScreen.cpp" generated)
"@
[IO.File]::WriteAllText("$scratch/generate.cmake", $script)
& cmake -P "$scratch/generate.cmake"
if ($LASTEXITCODE -ne 0) { throw 'Production save source generation failed' }
$generated = Get-Content -Raw "$scratch/generated/kernel_loadSavegameScreen.cpp"
$panel = [regex]::Match($generated, '(?s)struct saveMenuSubTask0 .*?static void Draw\(saveMenuSubTask0\* pThis\)\s*\{(.*?)\s*pThis->m1C_posX =')
if (!$panel.Success) { throw 'Native panel Draw boundary changed' }
$cpp = @"
#include <cassert>
struct saveMenuSubTask0 { int m0=-1,m4_delay=0,m10_status=0,m14=0,m18=0; };
void tick(saveMenuSubTask0* pThis) {
$($panel.Groups[1].Value)
}
int main() {
 saveMenuSubTask0 p;
 tick(&p); assert(p.m10_status==0 && p.m18==0x120);
 p.m0=0; p.m4_delay=2;
 tick(&p); assert(p.m18==0x120 && p.m4_delay==1);
 tick(&p); assert(p.m18==0x120 && p.m4_delay==0);
 for(int i=1;i<=9;++i) {tick(&p); assert(p.m18==0x120-i*0x20);}
 tick(&p); assert(p.m10_status==2 && p.m18==0);
 for(int i=0;i<20;++i) {tick(&p); assert(p.m18==0 && p.m10_status==2);}
 p.m0=-1; p.m4_delay=2;
 tick(&p); assert(p.m10_status==3 && p.m14==9 && p.m4_delay==1);
 tick(&p); assert(p.m18==0 && p.m4_delay==0);
 for(int i=1;i<=9;++i) {tick(&p); assert(p.m18==i*0x20);}
 tick(&p); assert(p.m10_status==0 && p.m18==0x120);
 tick(&p); assert(p.m10_status==0 && p.m18==0x120);
 p.m0=0; tick(&p); assert(p.m10_status==1 && p.m18==0x100);
}
"@
[IO.File]::WriteAllText("$scratch/test.cpp", $cpp)
& cl /nologo /std:c++20 /EHsc "$scratch/test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if ($LASTEXITCODE -ne 0) { throw 'Panel fixture compilation failed' }
& "$scratch/test.exe"
if ($LASTEXITCODE -ne 0) { throw 'Panel timing assertions failed' }
Write-Output 'PASS: actual generated save-panel opening, closing, delay, idle and re-entry; no hardware timing claim.'

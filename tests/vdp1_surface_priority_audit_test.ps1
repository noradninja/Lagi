$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-surface-priority-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
& cl /nologo /std:c++20 /EHsc "/I$repo/include" "$PSScriptRoot/vdp1_surface_priority_audit_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE){throw 'VDP1 surface priority fixture compilation failed'}
& "$scratch/test.exe"
if($LASTEXITCODE){throw 'VDP1 surface priority fixture failed'}
$renderer=Get-Content -Raw "$repo/src/platform/vita/neptune_renderer.cpp"
$inc=Get-Content -Raw "$repo/src/platform/vita/neptune_sprite_composite.inc"
if(!$renderer.Contains('#include "lagi/vdp1_surface_priority_audit.h"') -or
   !$inc.Contains('[VDP1SurfacePriority]') -or
   !$inc.Contains('sceGxmFinish(g_probeContext);')) {
    throw 'Completed-surface audit is not integrated'
}
Write-Output 'PASS diagnostic integration and selector fixture; Vita readback still requires hardware validation'

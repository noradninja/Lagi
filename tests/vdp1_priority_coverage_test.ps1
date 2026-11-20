$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-priority-coverage-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
& cl /nologo /std:c++20 /EHsc "/I$repo/include" "$PSScriptRoot/vdp1_priority_coverage_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE){throw 'Priority coverage host compilation failed'}
& "$scratch/test.exe"
if($LASTEXITCODE){throw 'Priority coverage host test failed'}
$renderer=Get-Content -Raw "$repo/src/platform/vita/neptune_renderer.cpp"
if(!$renderer.Contains('lagi::rbgLayerCompositeSupported(g_spriteColor) &&') -or
   !$renderer.Contains('[NativePriorityCoverage]') -or
   !$renderer.Contains('gpu.nativeSelectorMask = lagi::vdp1PrioritySelectorCoverage(')){
    throw 'Native renderer priority coverage path is missing'
}
Write-Output 'PASS priority audit instrumentation, no priority overrides'

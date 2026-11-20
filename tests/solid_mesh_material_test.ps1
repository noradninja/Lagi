param([string]$Compiler=$env:PSP2CGC)
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-solid-mesh-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
foreach($composite in 0,1){
    & cl /nologo /O2 /std:c++20 /EHsc "/DLAGI_VDP2_SPRITE_COMPOSITE=$composite" "$PSScriptRoot/solid_mesh_material_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
    if($LASTEXITCODE -ne 0){throw 'Solid mesh material fixture compile failed'}
    & "$scratch/test.exe"
    if($LASTEXITCODE -ne 0){throw 'Solid mesh material fixture failed'}
}
& $Compiler -profile sce_fp_psp2 "$repo/shaders/lagi_color_mesh_f.cg" -Wperf -fastprecision -O3 -xmlsdb "$scratch/mesh.xml" -o "$scratch/mesh.gxp"
if($LASTEXITCODE -ne 0){throw 'Solid mesh GPU compilation failed'}
[xml]$metadata=Get-Content -Raw "$scratch/mesh.xml"
if(@($metadata.sdb.resourceInfo.sampler).Where({$_}).Count -ne 0 -or @($metadata.sdb.resourceInfo.uniform).Where({$_}).Count -ne 0){throw 'Solid mesh unexpectedly needs texture/uniform resources'}
$renderer=Get-Content -Raw "$repo/src/platform/vita/neptune_renderer.cpp"
$start=$renderer.IndexOf('if (commandType == 4u || commandType == 6u)')
$end=$renderer.IndexOf('// VDP1 polyline.',$start)
$solid=$renderer.Substring($start,$end-$start)
if(!$solid.Contains('nativeSolidFragmentProgram(command.cmdPmod)') -or !$solid.Contains('commandZ(i)')){throw 'Native solid material selection or world-depth support lost'}
foreach($anchor in @('g_solidMeshFragmentRegistered=true;','&g_solidMeshFragmentProgram)<0','createNativeBlend(BlendSolidMesh,g_solidMeshFragmentProgramId,vertexProgram,kMultisampleMode)','ReleaseFragmentProgram(g_probeShaderPatcher,g_solidMeshFragmentProgram)','UnregisterProgram(g_probeShaderPatcher,g_solidMeshFragmentProgramId)')){
    if(!$renderer.Contains($anchor)){throw "Missing solid mesh shader lifetime/binding: $anchor"}
}
Write-Output 'PASS solid mesh shader reflection and GXM selection/lifetime bindings; hardware/performance acceptance pending'

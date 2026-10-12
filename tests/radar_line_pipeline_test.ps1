$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
$source = Get-Content -Raw (Join-Path $repo 'src/platform/vita/neptune_renderer.cpp')
$begin = $source.IndexOf('if ((command.fieldRadar && commandType == 4u) || commandType == 6u)')
$end = $source.IndexOf('// VDP1 polyline.', $begin)
if ($begin -lt 0 -or $end -le $begin) { throw 'Native solid/line renderer branch missing' }
$branch = $source.Substring($begin,$end-$begin)
$draw = $branch.IndexOf('sceGxmDraw(')
$restoreProgram = $branch.IndexOf('sceGxmSetVertexProgram(g_probeContext, g_textureVertexProgram)')
foreach ($face in @('Front','Back')) {
    $lineState = $branch.IndexOf("sceGxmSet${face}PolygonMode(g_probeContext, solidPolygonMode)")
    $fillState = $branch.IndexOf("sceGxmSet${face}PolygonMode(g_probeContext, SCE_GXM_POLYGON_MODE_TRIANGLE_FILL)")
    if ($lineState -lt 0 -or $lineState -ge $draw -or $fillState -le $draw -or $fillState -ge $restoreProgram) {
        throw "Native line $face raster state/restore ordering incorrect"
    }
}
if ($branch -notmatch 'commandType == 6u\s*\? SCE_GXM_POLYGON_MODE_LINE : SCE_GXM_POLYGON_MODE_TRIANGLE_FILL' -or
    $branch -notmatch 'SCE_GXM_PRIMITIVE_LINES' -or $branch -notmatch 'g_vdp1UiLineIndices') { throw 'Native line selection changed' }
Write-Output 'PASS: native CMD6 front/back line raster state precedes draw and fill state restored before sprites (source guard, not GPU execution)'

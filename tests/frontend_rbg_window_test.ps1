$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$source=Get-Content -Raw "$repo/src/platform/vita/neptune_renderer.cpp"
$start=$source.IndexOf('bool frontend_present_vdp2(')
$end=$source.IndexOf('void set_azel_color_offset_state(', $start)
$frontend=$source.Substring($start,$end-$start)
$release=$frontend.IndexOf('freeMovieResources();')
$vertices=$frontend.IndexOf('g_vdp2WindowVertices = static_cast<')
$indices=$frontend.IndexOf('g_vdp2WindowIndices = static_cast<')
$guard=$frontend.IndexOf('!g_vdp2WindowVertices || !g_vdp2WindowIndices')
$publish=$frontend.IndexOf('renderSlot.publish()')
if($release -lt 0 -or $vertices -lt $release -or $indices -lt $vertices -or
   $guard -lt $indices -or $publish -lt $guard){
    throw 'Raw frontend must recreate and validate RPMD3 coverage after resource replacement'
}
$draw=$source.Substring($source.IndexOf('static bool drawVdp2Rbg0Gpu('))
if(!$draw.Contains('if (rpmd == 3u && (!g_vdp2WindowVertices || !g_vdp2WindowIndices))')){
    throw 'Missing coverage must not silently select fullscreen A'
}
if($draw -notmatch 'int xs = 1;\s+int xe = 0;\s+if \(xeRaw != 0xFFFFu\)'){
    throw 'Disabled line window must be empty, not a one-pixel A span'
}
Write-Output 'PASS frontend RPMD3 allocation, failure guard, and disabled-window source contracts (not hardware validation)'

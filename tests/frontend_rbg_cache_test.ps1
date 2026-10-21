$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$s=Get-Content -Raw "$repo/src/platform/vita/neptune_renderer.cpp"
$f=$s.Substring($s.IndexOf('bool frontend_present_vdp2('))
foreach($part in @('if(LAGI_RBG_DECODED_PAGES && layout!=0u)',
    'const unsigned char* source=offset<vramBytes?vram+offset:cram+offset-vramBytes;',
    'std::memcmp(g_frontendVdp2RawShadow+offset,source,bytes)',
    'if(++g_frontendVdp2Generations[block]==0)++g_frontendVdp2Generations[block];',
    'g_frontendRbgDecodedReady=prepareRbgDecodedPagesFor(g_movieRbg0,',
    'g_frontendVdp2RawShadow,g_frontendVdp2Generations,true);')) {
    if(!$f.Contains($part)){throw "Frontend cache source contract missing: $part"}
}
$reset=$s.Substring($s.IndexOf('static void freeMovieResources()' + "`r`n" + '{'),300)
if(!$reset.Contains('g_frontendRbgDecodedReady=false;') -or !$reset.Contains('g_frontendVdp2RawInitialized=false;')){
    throw 'Resource replacement must invalidate frontend readiness/history'
}
$draw=$s.Substring($s.IndexOf('static bool drawVdp2Rbg0Gpu('),1800)
if(!$draw.Contains('decodedPages,') -or !$draw.Contains('!frontend && g_rbgCoefficientRowsReady')){
    throw 'Frontend page cache must retain its per-pixel coefficient path'
}
Write-Output 'PASS frontend cached-memory snapshot, generation wrap, lifetime and coefficient-path source contracts'

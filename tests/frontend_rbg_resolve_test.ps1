$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$s=Get-Content -Raw "$repo/src/platform/vita/neptune_renderer.cpp"
$f=$s.Substring($s.IndexOf('bool frontend_present_vdp2('))
if($f -notmatch 'g_movieVertices = static_cast<azel::DebugTextureVertex\*>\(\s+probeGpuAlloc\(\s+8u \* sizeof' -or
   !$f.Contains('g_movieVertices[7] = { 1,-1, 0.5f, 1, 1};')){
    throw 'Native frontend resolve quad must have mapped storage separate from presentation quad'
}
$r=$s.Substring($s.IndexOf('static bool renderSceneVdp2BackgroundPass(bool frontend = false)'))
foreach($fragment in @('frontend ? g_movieRbg0 : g_sceneRbg0',
    'frontend ? g_movieTextureData : g_sceneVdp2Raw',
    'frontend ? g_movieVertices + 4 : g_sceneVdp2Vertices',
    'frontend ? g_movieIndices : g_sceneVdp2Indices, 1.0f, frontend',
    'static_cast<int>(kSceneRbg0Width) - 1',
    'static_cast<int>(kSceneRbg0Height) - 1')){
    if(!$r.Contains($fragment)){throw "Shared native resolve contract lost: $fragment"}
}
$m=$s.Substring($s.IndexOf('static bool renderMovieFrame()' + "`r`n" + '{'))
$resolve=$m.IndexOf('renderSceneVdp2BackgroundPass(true)')
$begin=$m.IndexOf('const int beginResult = sceGxmBeginScene(')
if($resolve -lt 0 -or $resolve -gt $begin){throw 'Resolve must precede presentation scene'}
if($m.Contains('drawVdp2Rbg0Gpu(')){throw 'Expensive raw RBG decode returned to display-resolution scene'}
$start=$m.IndexOf('// Live RBG + NBG were decoded/composited at native size.')
$body=$m.Substring($start,$m.IndexOf('} else if (streamReady)', $start)-$start)
if(!$body.Contains('&g_sceneRbg0ResolvedTexture') -or
   !$body.Contains('setStochastic(kSceneRbg0Width, kSceneRbg0Height)') -or
   $body.Contains('g_vdp2NbgFragmentProgram')){throw 'Native composited presentation contract lost'}
Write-Output 'PASS frontend native-resolve/presentation source contracts (Vita timing and pixels unverified)'

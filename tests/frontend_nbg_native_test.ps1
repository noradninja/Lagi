$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-native-nbg-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
& cl /nologo /O2 /std:c++20 /EHsc "$PSScriptRoot/frontend_nbg_native_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Native NBG fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Native NBG fixture failed'}
$s=Get-Content -Raw "$repo/src/platform/vita/neptune_renderer.cpp"
$start=$s.IndexOf('static bool renderSceneVdp2BackgroundPass(bool frontend = false)')
$end=$s.IndexOf('static void drawSceneVdp2Background()', $start)
$resolve=$s.Substring($start,$end-$start)
$rbg=$resolve.IndexOf('submitted = drawVdp2Rbg0Gpu(')
$nbg=$resolve.IndexOf('if(submitted && frontend)submitted=drawFrontendNbgNative();')
$finish=$resolve.IndexOf('sceGxmEndScene(')
if($rbg -lt 0 -or $nbg -lt $rbg -or $finish -lt $nbg){throw 'NBG must follow RBG/color offset within native resolve scene'}
$start=$s.IndexOf('static bool renderMovieFrame()' + "`r`n" + '{')
$movie=$s.Substring($start)
$d5=$movie.IndexOf('// Live RBG + NBG were decoded/composited at native size.')
$d5Body=$movie.Substring($d5,$movie.IndexOf('} else if (streamReady)', $d5)-$d5)
if($d5Body.Contains('g_vdp2NbgFragmentProgram')){
    throw 'Raw NBG decoding returned to enlarged presentation scene'
}
if(!$movie.Contains('else if (streamReady && g_movieUsesVdp2Title && g_movieVdp2Info[0] < 0.5f)')){
    throw 'Raw title setup fallback must remain separate'
}
if(!$movie.Contains('layout=%u nbg=%u')){throw 'Frontend timing must identify native NBG visibility'}
Write-Output 'PASS native resolve ordering, no enlarged NBG decode, and visibility timing contracts'

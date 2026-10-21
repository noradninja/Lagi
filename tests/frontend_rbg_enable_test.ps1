$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$s=Get-Content -Raw "$repo/src/platform/vita/neptune_renderer.cpp"
if(!$s.Contains('rbg.enabled=(state.bgon&0x10u)!=0;')){throw 'Frontend must retain native RBG enable bit'}
$start=$s.IndexOf('static bool renderSceneVdp2BackgroundPass(bool frontend = false)')
$end=$s.IndexOf('static void drawSceneVdp2Background()', $start)
$resolve=$s.Substring($start,$end-$start)
$clear=$resolve.IndexOf('std::memset(')
$gate=$resolve.IndexOf('const bool rbgEnabled=!frontend || g_movieRbg0.enabled;')
$draw=$resolve.IndexOf('submitted = drawVdp2Rbg0Gpu(')
$nbg=$resolve.IndexOf('if(submitted && frontend)submitted=drawFrontendNbgNative();')
if($clear -lt 0 -or $gate -lt $clear -or $draw -lt $gate -or $nbg -lt $draw -or
   !$resolve.Contains('bool submitted = !rbgEnabled;') -or
   !$resolve.Contains('if (rbgEnabled && sceGxmReserveVertexDefaultUniformBuffer(')){
    throw 'Disabled frontend must keep cleared native target and still allow NBG without decoding RBG'
}
if(!$s.Contains('if(frontendRbg && g_movieRbg0.enabled && g_frontendVdp2RawInitialized)')){
    throw 'Disabled RBG must not rebuild decoded page cache from reused VRAM'
}
Write-Output 'PASS frontend native BGON gating/order source contracts; exit pixels require device test'

param([string]$Compiler=$env:PSP2CGC)
$ErrorActionPreference='Stop'
if(!$Compiler -or !(Test-Path -LiteralPath $Compiler)){throw 'Set PSP2CGC to the installed Vita shader compiler'}
$repo=Split-Path $PSScriptRoot -Parent
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-rbg-resources-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
$bindings=Get-Content -Raw "$repo/src/platform/vita/neptune_rbg_tile_program.inc"
$block=[regex]::Match($bindings,'const char\* names\[22\]=\{([\s\S]*?)\};').Groups[1].Value
$names=@([regex]::Matches($block,'"(rbg0\w+)"') | ForEach-Object {$_.Groups[1].Value})
if($names.Count -ne 22){throw 'Runtime reflection list must contain 22 bindings'}
foreach($variant in @('vdp2_rbg0_f','vdp2_rbg0_tile16_8bit_f','vdp2_rbg0_pages_f')) {
    $xmlPath=Join-Path $scratch "$variant.xml"
    & $Compiler -profile sce_fp_psp2 "$repo/shaders/$variant.cg" -Wperf -fastprecision -O3 -xmlsdb $xmlPath -o (Join-Path $scratch "$variant.gxp")
    if($LASTEXITCODE -ne 0){throw "Shader compilation failed: $variant"}
    [xml]$metadata=Get-Content -Raw $xmlPath
    $resources=$metadata.sdb.resourceInfo
    foreach($name in $names) {
        $uniform=@($resources.uniform | Where-Object {$_.symbolName -eq $name})
        if($uniform.Count -ne 1 -or $uniform[0].leafFormat -ne 'fp32:4' -or $uniform[0].buffer -ne 'default') {
            throw "Missing or incompatible reflected uniform $name in $variant"
        }
    }
    $raw=@($resources.sampler | Where-Object {$_.symbolName -eq 'raw'})
    if($raw.Count -ne 1 -or $raw[0].textureUnit -ne '0'){throw "Raw sampler binding changed in $variant"}
    $bytes=@($resources.sampler | Where-Object {$_.symbolName -eq 'rawBytes'})
    if($variant -eq 'vdp2_rbg0_tile16_8bit_f') {
        if($bytes.Count -ne 1 -or $bytes[0].textureUnit -ne '1'){throw 'Byte-view sampler must be on texture unit 1'}
    } elseif($bytes.Count -ne 0){throw 'Generic/frontend shader must not require a byte-view texture'}
    $decoded=@($resources.sampler | Where-Object {$_.symbolName -eq 'decodedPages'})
    $lookup=@($resources.sampler | Where-Object {$_.symbolName -eq 'pageSlots'})
    if($variant -eq 'vdp2_rbg0_pages_f') {
        if($decoded.Count -ne 1 -or $decoded[0].textureUnit -ne '1' -or $lookup.Count -ne 0){throw 'Decoded-page texture slots changed'}
    } elseif($decoded.Count -ne 0 -or $lookup.Count -ne 0){throw 'Non-cache shader requires page resources'}
    Write-Output "PASS $variant compiler reflection: 22 uniforms and expected sampler slots"
}

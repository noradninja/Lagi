$ErrorActionPreference = 'Stop'

$rawBytes = 0x81000
$blockBytes = 4096
$source = [byte[]]::new($rawBytes)
$shadow = [byte[]]::new($rawBytes)
$gpu = [byte[]]::new($rawBytes)
$initialized = $false

function Update-RawShadow {
    param(
        [byte[]]$Current,
        [byte[]]$CpuShadow,
        [byte[]]$GpuStorage,
        [ref]$Initialized
    )

    $updated = 0
    for ($offset = 0; $offset -lt $Current.Length; $offset += $blockBytes) {
        $bytes = [Math]::Min($blockBytes, $Current.Length - $offset)
        $changed = -not $Initialized.Value
        if (-not $changed) {
            for ($i = 0; $i -lt $bytes; ++$i) {
                if ($CpuShadow[$offset + $i] -ne $Current[$offset + $i]) {
                    $changed = $true
                    break
                }
            }
        }
        if ($changed) {
            [Buffer]::BlockCopy($Current, $offset, $GpuStorage, $offset, $bytes)
            [Buffer]::BlockCopy($Current, $offset, $CpuShadow, $offset, $bytes)
            $updated += $bytes
        }
    }
    $Initialized.Value = $true
    return $updated
}

function Assert-BytesEqual {
    param([byte[]]$Expected, [byte[]]$Actual, [string]$Message)
    if ($Expected.Length -ne $Actual.Length) { throw $Message }
    for ($i = 0; $i -lt $Expected.Length; ++$i) {
        if ($Expected[$i] -ne $Actual[$i]) { throw "$Message at byte $i" }
    }
}

for ($i = 0; $i -lt $source.Length; ++$i) {
    $source[$i] = ($i * 29 + 17) -band 255
}

$updated = Update-RawShadow $source $shadow $gpu ([ref]$initialized)
if ($updated -ne $rawBytes) { throw "Initial upload was $updated bytes" }
Assert-BytesEqual $source $shadow 'Initial shadow mismatch'
Assert-BytesEqual $source $gpu 'Initial GPU copy mismatch'

$updated = Update-RawShadow $source $shadow $gpu ([ref]$initialized)
if ($updated -ne 0) { throw "Identical frame updated $updated bytes" }

$source[2 * $blockBytes + 31] = $source[2 * $blockBytes + 31] -bxor 0xff
$updated = Update-RawShadow $source $shadow $gpu ([ref]$initialized)
if ($updated -ne $blockBytes) { throw "Single block updated $updated bytes" }

$source[$rawBytes - 1] = $source[$rawBytes - 1] -bxor 0xff
$updated = Update-RawShadow $source $shadow $gpu ([ref]$initialized)
if ($updated -ne $blockBytes) { throw "Final VDP2 block updated $updated bytes" }

# Exercise the helper's bounded-tail behavior independently; kRawVdp2Bytes is
# currently exactly block-aligned, but the loop must remain correct if it is not.
$partialLength = 2 * $blockBytes + 37
$partialSource = [byte[]]::new($partialLength)
$partialShadow = [byte[]]::new($partialLength)
$partialGpu = [byte[]]::new($partialLength)
$partialInitialized = $true
$partialSource[$partialLength - 1] = 0x5a
$updated = Update-RawShadow $partialSource $partialShadow $partialGpu ([ref]$partialInitialized)
if ($updated -ne 37) { throw "Synthetic final partial block updated $updated bytes" }

# Rotation/window state is intentionally outside this raw-memory helper. An
# unchanged snapshot must transfer zero bytes while the caller still resolves.
$updated = Update-RawShadow $source $shadow $gpu ([ref]$initialized)
if ($updated -ne 0) { throw "Parameter-only frame updated $updated bytes" }

# GPU resource recreation invalidates the shadow relationship even when the
# byte contents happen to be identical.
$initialized = $false
[Array]::Clear($gpu)
$updated = Update-RawShadow $source $shadow $gpu ([ref]$initialized)
if ($updated -ne $rawBytes) { throw "Recreated GPU upload was $updated bytes" }
Assert-BytesEqual $source $gpu 'Recreated GPU copy mismatch'

$renderer = Get-Content "$PSScriptRoot\..\src\platform\vita\neptune_renderer.cpp" -Raw
if ($renderer -match 'memcmp\s*\(\s*gpu\s*\+') {
    throw 'Renderer still compares against GPU-mapped memory'
}
if ($renderer -notmatch 'memcmp\s*\(\s*g_sceneVdp2GpuRawShadow\s*\+') {
    throw 'Renderer does not compare against the CPU shadow'
}
if ($renderer -notmatch 'memcpy\s*\(\s*g_sceneVdp2GpuRawShadow\s*\+') {
    throw 'Renderer does not update the CPU shadow after upload'
}

Write-Output 'PASS: VDP2 CPU shadow initial, unchanged, changed-block, tail, and recreation cases'

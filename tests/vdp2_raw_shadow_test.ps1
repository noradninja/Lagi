$ErrorActionPreference = 'Stop'

$rawBytes = 0x81000
$blockBytes = 4096
$source = [byte[]]::new($rawBytes)
$producerShadow = [byte[]]::new($rawBytes)
$gpu = [byte[]]::new($rawBytes)
$blockCount = [int](($rawBytes + $blockBytes - 1) / $blockBytes)
$stagedGenerations = [uint32[]]::new($blockCount)
$publishedGenerations = [uint32[]]::new($blockCount)
$uploadedGenerations = [uint32[]]::new($blockCount)
$producerInitialized = $false
$gpuInitialized = $false

function Stage-RawSnapshot {
    param(
        [byte[]]$Current,
        [byte[]]$ProducerShadow,
        [uint32[]]$StagedGenerations,
        [uint32[]]$PublishedGenerations,
        [ref]$Initialized
    )

    $block = 0
    for ($offset = 0; $offset -lt $Current.Length; $offset += $blockBytes) {
        $bytes = [Math]::Min($blockBytes, $Current.Length - $offset)
        $changed = -not $Initialized.Value
        if (-not $changed) {
            for ($i = 0; $i -lt $bytes; ++$i) {
                if ($ProducerShadow[$offset + $i] -ne $Current[$offset + $i]) {
                    $changed = $true
                    break
                }
            }
        }
        if ($changed) {
            [Buffer]::BlockCopy($Current, $offset, $ProducerShadow, $offset, $bytes)
            ++$StagedGenerations[$block]
            if ($StagedGenerations[$block] -eq 0) { ++$StagedGenerations[$block] }
        }
        $PublishedGenerations[$block] = $StagedGenerations[$block]
        ++$block
    }
    $Initialized.Value = $true
}

function Upload-PublishedSnapshot {
    param(
        [byte[]]$Current,
        [byte[]]$GpuStorage,
        [uint32[]]$PublishedGenerations,
        [uint32[]]$UploadedGenerations,
        [ref]$Initialized
    )

    $updated = 0
    $block = 0
    for ($offset = 0; $offset -lt $Current.Length; $offset += $blockBytes) {
        $bytes = [Math]::Min($blockBytes, $Current.Length - $offset)
        if (-not $Initialized.Value -or
            $UploadedGenerations[$block] -ne $PublishedGenerations[$block]) {
            [Buffer]::BlockCopy($Current, $offset, $GpuStorage, $offset, $bytes)
            $UploadedGenerations[$block] = $PublishedGenerations[$block]
            $updated += $bytes
        }
        ++$block
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

Stage-RawSnapshot $source $producerShadow $stagedGenerations $publishedGenerations ([ref]$producerInitialized)
$updated = Upload-PublishedSnapshot $source $gpu $publishedGenerations $uploadedGenerations ([ref]$gpuInitialized)
if ($updated -ne $rawBytes) { throw "Initial upload was $updated bytes" }
Assert-BytesEqual $source $producerShadow 'Initial producer shadow mismatch'
Assert-BytesEqual $source $gpu 'Initial GPU copy mismatch'

Stage-RawSnapshot $source $producerShadow $stagedGenerations $publishedGenerations ([ref]$producerInitialized)
$updated = Upload-PublishedSnapshot $source $gpu $publishedGenerations $uploadedGenerations ([ref]$gpuInitialized)
if ($updated -ne 0) { throw "Identical frame updated $updated bytes" }

$source[2 * $blockBytes + 31] = $source[2 * $blockBytes + 31] -bxor 0xff
Stage-RawSnapshot $source $producerShadow $stagedGenerations $publishedGenerations ([ref]$producerInitialized)
$updated = Upload-PublishedSnapshot $source $gpu $publishedGenerations $uploadedGenerations ([ref]$gpuInitialized)
if ($updated -ne $blockBytes) { throw "Single block updated $updated bytes" }

$source[$rawBytes - 1] = $source[$rawBytes - 1] -bxor 0xff
Stage-RawSnapshot $source $producerShadow $stagedGenerations $publishedGenerations ([ref]$producerInitialized)
$updated = Upload-PublishedSnapshot $source $gpu $publishedGenerations $uploadedGenerations ([ref]$gpuInitialized)
if ($updated -ne $blockBytes) { throw "Final VDP2 block updated $updated bytes" }

# Exercise the helper's bounded-tail behavior independently; kRawVdp2Bytes is
# currently exactly block-aligned, but the loop must remain correct if it is not.
$partialLength = 2 * $blockBytes + 37
$partialSource = [byte[]]::new($partialLength)
$partialShadow = [byte[]]::new($partialLength)
$partialGpu = [byte[]]::new($partialLength)
$partialBlockCount = [int](($partialLength + $blockBytes - 1) / $blockBytes)
$partialStaged = [uint32[]]::new($partialBlockCount)
$partialPublished = [uint32[]]::new($partialBlockCount)
$partialUploaded = [uint32[]]::new($partialBlockCount)
$partialProducerInitialized = $false
$partialGpuInitialized = $false
$partialSource[$partialLength - 1] = 0x5a
Stage-RawSnapshot $partialSource $partialShadow $partialStaged $partialPublished ([ref]$partialProducerInitialized)
$updated = Upload-PublishedSnapshot $partialSource $partialGpu $partialPublished $partialUploaded ([ref]$partialGpuInitialized)
if ($updated -ne $partialLength) { throw "Initial partial upload was $updated bytes" }
$partialSource[$partialLength - 1] = 0xa5
Stage-RawSnapshot $partialSource $partialShadow $partialStaged $partialPublished ([ref]$partialProducerInitialized)
$updated = Upload-PublishedSnapshot $partialSource $partialGpu $partialPublished $partialUploaded ([ref]$partialGpuInitialized)
if ($updated -ne 37) { throw "Synthetic final partial block updated $updated bytes" }

# Rotation/window state is intentionally outside this raw-memory helper. An
# unchanged snapshot must transfer zero bytes while the caller still resolves.
Stage-RawSnapshot $source $producerShadow $stagedGenerations $publishedGenerations ([ref]$producerInitialized)
$updated = Upload-PublishedSnapshot $source $gpu $publishedGenerations $uploadedGenerations ([ref]$gpuInitialized)
if ($updated -ne 0) { throw "Parameter-only frame updated $updated bytes" }

# GPU resource recreation invalidates the shadow relationship even when the
# byte contents happen to be identical.
$gpuInitialized = $false
[Array]::Clear($gpu)
$updated = Upload-PublishedSnapshot $source $gpu $publishedGenerations $uploadedGenerations ([ref]$gpuInitialized)
if ($updated -ne $rawBytes) { throw "Recreated GPU upload was $updated bytes" }
Assert-BytesEqual $source $gpu 'Recreated GPU copy mismatch'

# Model the ownership boundary independently of the byte comparison: producer
# staging must leave the current renderer snapshot untouched, even when the
# alternating pending buffer contains an older value of a reverted block.
$fixtureBytes = 2 * $blockBytes
$fixtureSource = [byte[]]::new($fixtureBytes)
$fixtureShadow = [byte[]]::new($fixtureBytes)
$fixtureGpu = [byte[]]::new($fixtureBytes)
$pendingRaw = [byte[]]::new($fixtureBytes)
$currentRaw = [byte[]]::new($fixtureBytes)
$fixtureStaged = [uint32[]]::new(2)
$pendingGenerations = [uint32[]]::new(2)
$currentGenerations = [uint32[]]::new(2)
$fixtureUploaded = [uint32[]]::new(2)
$fixtureProducerInitialized = $false
$fixtureGpuInitialized = $false
foreach ($value in @(0, 0x5a, 0, 0xa5, 0xa5, 0)) {
    $fixtureSource[17] = $value
    $oldCurrentValue = $currentRaw[17]
    $oldCurrentGeneration = $currentGenerations[0]
    [Buffer]::BlockCopy($fixtureSource, 0, $pendingRaw, 0, $fixtureBytes)
    Stage-RawSnapshot $pendingRaw $fixtureShadow $fixtureStaged $pendingGenerations ([ref]$fixtureProducerInitialized)
    if ($currentRaw[17] -ne $oldCurrentValue -or
        $currentGenerations[0] -ne $oldCurrentGeneration) {
        throw 'Producer staging modified the renderer-owned snapshot'
    }
    $oldGpuValue = $fixtureGpu[17]
    $wasInitialized = $fixtureGpuInitialized
    $swapRaw = $currentRaw
    $currentRaw = $pendingRaw
    $pendingRaw = $swapRaw
    $swapGenerations = $currentGenerations
    $currentGenerations = $pendingGenerations
    $pendingGenerations = $swapGenerations
    $updated = Upload-PublishedSnapshot $currentRaw $fixtureGpu $currentGenerations $fixtureUploaded ([ref]$fixtureGpuInitialized)
    $expectedBytes = if (-not $wasInitialized) { $fixtureBytes }
        elseif ($oldGpuValue -ne $value) { $blockBytes } else { 0 }
    if ($updated -ne $expectedBytes) {
        throw "Alternating-buffer value $value updated $updated bytes, expected $expectedBytes"
    }
    Assert-BytesEqual $fixtureSource $fixtureGpu 'Alternating snapshot/reversion mismatch'
}

# Multiple staging calls before publication must still select the newest bytes.
# Reverting between calls advances the generation rather than comparing against
# the two-frames-old pending buffer and accidentally skipping the upload.
$fixtureSource[17] = 0x5a
[Buffer]::BlockCopy($fixtureSource, 0, $pendingRaw, 0, $fixtureBytes)
Stage-RawSnapshot $pendingRaw $fixtureShadow $fixtureStaged $pendingGenerations ([ref]$fixtureProducerInitialized)
$fixtureSource[17] = 0
[Buffer]::BlockCopy($fixtureSource, 0, $pendingRaw, 0, $fixtureBytes)
Stage-RawSnapshot $pendingRaw $fixtureShadow $fixtureStaged $pendingGenerations ([ref]$fixtureProducerInitialized)
$updated = Upload-PublishedSnapshot $pendingRaw $fixtureGpu $pendingGenerations $fixtureUploaded ([ref]$fixtureGpuInitialized)
if ($updated -ne $blockBytes) { throw 'Unpublished change/reversion lost its generation' }
Assert-BytesEqual $fixtureSource $fixtureGpu 'Newest staged snapshot mismatch'

$renderer = Get-Content "$PSScriptRoot\..\src\platform\vita\neptune_renderer.cpp" -Raw
if ($renderer -match 'memcmp\s*\(\s*gpu\s*\+') {
    throw 'Renderer still compares against GPU-mapped memory'
}
if ($renderer -match 'memcmp\s*\(\s*g_sceneVdp2GpuRawShadow\s*\+') {
    throw 'Render thread still scans the entire CPU shadow'
}
if ($renderer -notmatch 'memcmp\s*\(\s*g_stagedSceneVdp2RawShadow\s*\+') {
    throw 'Producer does not compare staged raw blocks exactly'
}
if ($renderer -notmatch 'g_uploadedSceneVdp2BlockGenerations\[block\]\s*!=') {
    throw 'Renderer does not select uploads from published block generations'
}
if ($renderer -notmatch 'swap\(g_sceneVdp2BlockGenerations,') {
    throw 'Published block generations do not follow snapshot ownership'
}

Write-Output 'PASS: VDP2 generations cover initial, unchanged, changes/reversions, alternating ownership, tail, and recreation'

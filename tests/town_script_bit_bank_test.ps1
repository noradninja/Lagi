param([Parameter(Mandatory=$true)][string]$DiscImage)
$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
$disc = [IO.File]::OpenRead((Resolve-Path -LiteralPath $DiscImage).Path)
try {
    function Read-MainWord([int]$address) {
        $word = 0
        for ($i = 0; $i -lt 2; $i++) {
            $offset = $address + $i - 0x06006000
            $disc.Position = (58 + [long][Math]::Floor($offset / 2048)) * 2352 + 16 + ($offset % 2048)
            $byte = $disc.ReadByte()
            if ($byte -lt 0) { throw 'Short MAIN read' }
            $word = ($word -shl 8) -bor $byte
        }
        $word
    }

    if ((Read-MainWord 0x0600D028) -ne 0x9811 -or
        (Read-MainWord 0x0600D04E) -ne 1000) { throw 'Town operand threshold differs' }
    $handlers = @(
        @(15, 0x0600D268, 0x0600D27A, 0x3483, 2, 0x8901, 0x0600D282),
        @(16, 0x0600D2A8, 0x0600D2BA, 0x3483, 2, 0x8901, 0x0600D2C2),
        @(17, 0x0600D2F4, 0x0600D306, 0x3483, 2, 0x8901, 0x0600D30E),
        @(18, 0x0600D334, 0x0600D348, 0x3383, 2, 0x8902, 0x0600D352),
        @(20, 0x0600D3A0, 0x0600D3BA, 0x3183, 10, 0x8901, 0x0600D3CA)
    )
    foreach ($handler in $handlers) {
        $opcode, $entry, $compare, $cmpWord, $branchDistance, $branchWord, $offsetLoad = $handler
        $slot = 0x0600D054 + 2 * ($opcode - 1)
        $loadWord = Read-MainWord $offsetLoad
        $offsetLiteral = $offsetLoad + 4 + 2 * ($loadWord -band 255)
        if (0x0600D04C + (Read-MainWord $slot) -ne $entry -or
            (Read-MainWord $compare) -ne $cmpWord -or
            (Read-MainWord ($compare + $branchDistance)) -ne $branchWord -or
            ($loadWord -shr 12) -ne 9 -or
            (Read-MainWord $offsetLiteral) -ne 3334) {
            throw "Original town opcode $opcode bit-bank selection differs"
        }
    }
} finally {
    $disc.Dispose()
}

$cmake = Get-Content -Raw (Join-Path $repo 'CMakeLists.txt')
$patch = [regex]::Match($cmake, '(?s)  # Saturn MAIN0600D028.*?  endforeach\(\)')
if (!$patch.Success) { throw 'Missing town interpreter adapter' }
$scratch = Join-Path ([IO.Path]::GetTempPath()) ('lagi-town-bits-' + [guid]::NewGuid())
New-Item -ItemType Directory -Path $scratch | Out-Null
try {
    $source = (Join-Path $repo 'extern/Azel/AzelLib/town/townScript.cpp').Replace('\', '/')
    $output = (Join-Path $scratch 'townScript.cpp').Replace('\', '/')
    $script = Join-Path $scratch 'adapter.cmake'
    @(
        ('file(READ "{0}" source_text)' -f $source),
        $patch.Value,
        ('file(WRITE "{0}" "${{source_text}}")' -f $output)
    ) | Set-Content -Encoding utf8 $script
    & cmake -P $script
    if ($LASTEXITCODE) { throw 'Town interpreter adapter failed' }
    $generated = Get-Content -Raw $output
    foreach ($opcode in 15, 16, 17, 18, 20) {
        $body = [regex]::Match($generated, "(?s)case ${opcode}:.*?(?=case \d+:)").Value
        $operand = if ($opcode -le 17) { 'r4' } else { 'var0' }
        if ($body -notmatch "if \($operand >= 1000\)\s*\{\s*$operand \+= 3334;") {
            throw "Generated town opcode $opcode does not match the native bit-bank rule"
        }
    }
} finally {
    Remove-Item -LiteralPath (Join-Path $scratch 'adapter.cmake'), (Join-Path $scratch 'townScript.cpp') -ErrorAction SilentlyContinue
    Remove-Item -LiteralPath $scratch
}
'PASS Disc 1 town bit operand threshold and all five generated opcode guards'

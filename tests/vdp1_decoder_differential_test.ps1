param([string]$DevShell = 'C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/Common7/Tools/Launch-VsDevShell.ps1')
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$current = Get-Content -Raw "$root/src/platform/vita/neptune_renderer.cpp"
$baseline = (& git -C $root show '386fff22889b8e63676e8d6d23177f36c1fd4f6f:src/platform/vita/neptune_renderer.cpp') -join "`n"
if ($LASTEXITCODE -ne 0) { throw 'Cannot read baseline decoder' }
function Select-Decoder([string]$Source, [string]$Name) {
    $start = $Source.LastIndexOf('static bool decodeLiveVdp1Texture(')
    $end = $Source.IndexOf('static std::uint16_t liveTownTextureIndex(', $start)
    if ($start -lt 0 -or $end -lt 0) { throw 'Decoder extraction boundary changed' }
    return $Source.Substring($start, $end - $start).Replace('decodeLiveVdp1Texture(', "$Name(").Replace('const unsigned char* textureMemory)', 'const unsigned char* textureMemory = nullptr)')
}
$preamble = @'
#include <cstdint>
#include <vector>
#include <cstdio>
namespace azel {
struct SaturnPolygonRecord {
    std::uint16_t cmdCtrl=2,cmdPmod=0,cmdColr=0,cmdSrca=1024,cmdSize=0;
    unsigned textureWidth() const { return ((cmdSize >> 8) & 63) * 8; }
    unsigned textureHeight() const { return cmdSize & 255; }
    unsigned colorMode() const { return (cmdPmod >> 3) & 7; }
};
struct DecodedMode1Texture {
    std::uint16_t cmdPmod,cmdColr,cmdSrca,cmdSize;
    unsigned width=0,height=0;
    std::vector<std::uint32_t> rgba;
};
}
static unsigned char vram[0x80000],g_vdp2Cram[4096];
static unsigned char* getVdp1Pointer(unsigned address) {
    unsigned offset=address-0x25C00000u;
    return offset<sizeof(vram)?vram+offset:nullptr;
}
static std::uint16_t readVdp2Be16(const unsigned char* data,unsigned offset) {
    return (std::uint16_t(data[offset])<<8)|data[offset+1];
}
// Both actual decoder bodies use this same deterministic color conversion.
// The test isolates changed decode/cache behavior, not converter correctness.
static std::uint32_t vdp2Rgb555ToAbgr(std::uint16_t c) {
    return 0xff000000u | ((c&31)*255/31) |
        (((c>>5)&31)*255/31<<8) | (((c>>10)&31)*255/31<<16);
}
namespace logging { template<class... Args> void writef(const char*,Args...) {} }
'@
$driver = @'
int main() {
    unsigned seed=12345,cases=0;
    for(unsigned pass=0;pass<3;++pass) {
        for(auto& b:vram) { seed=seed*1664525u+1013904223u; b=pass==0?0:pass==1?255:seed>>24; }
        for(auto& b:g_vdp2Cram) { seed=seed*1664525u+1013904223u; b=seed>>24; }
        // Palette/LUT zero entries explicitly exercise cached transparent results.
        g_vdp2Cram[0]=g_vdp2Cram[1]=0;
        const std::vector<unsigned char> snapshot(vram,vram+sizeof(vram));
        for(unsigned mode=0;mode<6;++mode)
        for(unsigned flags=0;flags<4;++flags)
        for(unsigned width: {8u,32u,128u})
        for(unsigned height: {1u,8u,32u})
        for(unsigned bank: {0u,16u,256u,2032u}) {
            azel::SaturnPolygonRecord record;
            record.cmdPmod=(mode<<3)|((flags&1)?0x40:0)|((flags&2)?0x80:0);
            record.cmdColr=bank;
            record.cmdSize=((width/8)<<8)|height;
            azel::DecodedMode1Texture reference,candidate;
            bool a=decodeReference(record,reference),b=decodeCandidate(record,candidate);
            if(a!=b || reference.width!=candidate.width || reference.height!=candidate.height ||
               reference.rgba!=candidate.rgba) {
                std::printf("FAIL mode=%u flags=%u width=%u height=%u bank=%u\n",mode,flags,width,height,bank);
                return 1;
            }
            ++cases;
            // Published decoding must not fall back to mutated live pixels/LUT.
            vram[8192]^=255;
            vram[bank<<3]^=255;
            azel::DecodedMode1Texture owned;
            bool c=decodeCandidate(record,owned,snapshot.data());
            vram[bank<<3]^=255;
            vram[8192]^=255;
            if(c!=a || owned.width!=reference.width || owned.height!=reference.height ||
               owned.rgba!=reference.rgba) {
                std::printf("FAIL immutable snapshot mode=%u bank=%u\n",mode,bank);
                return 1;
            }
        }
    }
    std::printf("PASS: %u decoder differential and immutable snapshot cases\n",cases);
}
'@
# Generated test artifacts, not repository source edits. Keep them for inspection.
$artifactDirectory = Join-Path $root 'build/host-vdp1-decode'
New-Item -ItemType Directory -Force -Path $artifactDirectory | Out-Null
$cpp = Join-Path $artifactDirectory 'differential.cpp'
$exe = Join-Path $artifactDirectory 'differential.exe'
[IO.File]::WriteAllText($cpp, $preamble + "`n" + (Select-Decoder $baseline 'decodeReference') +
    "`n" + (Select-Decoder $current 'decodeCandidate') + "`n" + $driver)
& $DevShell -Arch amd64 -HostArch amd64 -SkipAutomaticLocation | Out-Null
& cl.exe /nologo /std:c++17 /EHsc /O2 "/Fe:$exe" "/Fo:$artifactDirectory/" $cpp
if ($LASTEXITCODE -ne 0) { throw 'Host differential compilation failed' }
& $exe
if ($LASTEXITCODE -ne 0) { throw 'Host decoder differential failed' }

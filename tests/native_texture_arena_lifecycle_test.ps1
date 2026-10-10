param([string]$DevShell = 'C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/Common7/Tools/Launch-VsDevShell.ps1')
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$source = Get-Content -Raw "$root/src/platform/vita/neptune_renderer.cpp"
$start = $source.IndexOf('static void freeVdp1Textures(bool releaseArena)')
$end = $source.IndexOf('static bool ensureVdp1UiBuffers()', $start)
if ($start -lt 0 -or $end -lt 0) { throw 'Texture release extraction boundary changed' }
$preamble = @'
#include <algorithm>
#include <vector>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
using SceUID = int;
struct GpuMode1Texture { SceUID uid=-1; void* data=nullptr; };
struct FieldTextureSlab {
    SceUID uid=-1; void* data=nullptr; std::size_t bytes=0,used=0;
    bool startupArena=false;
};
static std::vector<GpuMode1Texture> g_vdp1GpuTextures;
static std::vector<int> g_nativeGpuTextureGenerations,g_vdp1TextureBatches;
static std::vector<FieldTextureSlab> g_fieldTextureSlabs;
static bool g_vdp1TexturedReady=true;
static std::vector<int> released;
static unsigned unmapped=0;
static void sceGxmUnmapMemory(void*) { ++unmapped; }
static void sceKernelFreeMemBlock(int uid) { released.push_back(uid); }
static void freeMovieMappedBlock(SceUID& uid,void*& memory) {
    if(uid>=0) {
        if(memory) sceGxmUnmapMemory(memory);
        sceKernelFreeMemBlock(uid);
    }
    uid=-1; memory=nullptr;
}
static void check(bool result,const char* message) {
    if(!result) { std::printf("FAIL: %s\n",message); std::exit(1); }
}
'@
$driver = @'
int main() {
    int storage[3]{};
    g_fieldTextureSlabs={{7,&storage[0],8*1024*1024,512,true},
                        {8,&storage[1],1024*1024,1024,false}};
    g_vdp1GpuTextures={{-1,&storage[0]},{9,&storage[2]}};
    g_nativeGpuTextureGenerations={1,1}; g_vdp1TextureBatches={1};
    freeVdp1Textures(false);
    check(g_fieldTextureSlabs.size()==1,"only startup slab retained");
    const auto& arena=g_fieldTextureSlabs.front();
    check(arena.uid==7 && arena.data==&storage[0] && arena.used==0 &&
          arena.bytes==8*1024*1024 && arena.startupArena,"arena reset without releasing mapping");
    check(released==std::vector<int>({9,8}) && unmapped==2,"owned texture and overflow freed once");
    check(g_vdp1GpuTextures.empty() && g_nativeGpuTextureGenerations.empty() &&
          g_vdp1TextureBatches.empty() && !g_vdp1TexturedReady,"descriptor state cleared");
    g_fieldTextureSlabs.front().used=2048;
    freeVdp1Textures(false);
    check(g_fieldTextureSlabs.front().used==0 && released.size()==2,"repeated scene release retains arena");
    freeVdp1Textures(true);
    check(g_fieldTextureSlabs.empty() && released==std::vector<int>({9,8,7}) &&
          unmapped==3,"shutdown releases arena once");
    freeVdp1Textures(true);
    check(released.size()==3 && unmapped==3,"repeated shutdown is harmless");
    std::puts("PASS: actual texture release lifecycle; mapped-memory calls mocked, not hardware validation");
}
'@
$artifactDirectory = Join-Path $root 'build/host-texture-arena'
New-Item -ItemType Directory -Force -Path $artifactDirectory | Out-Null
$cpp = Join-Path $artifactDirectory 'lifecycle.cpp'
$exe = Join-Path $artifactDirectory 'lifecycle.exe'
[IO.File]::WriteAllText($cpp, $preamble + "`n" + $source.Substring($start,$end-$start) + "`n" + $driver)
& $DevShell -Arch amd64 -HostArch amd64 -SkipAutomaticLocation | Out-Null
& cl.exe /nologo /std:c++17 /EHsc /O2 "/Fe:$exe" "/Fo:$artifactDirectory/" $cpp
if ($LASTEXITCODE -ne 0) { throw 'Lifecycle test compilation failed' }
& $exe
if ($LASTEXITCODE -ne 0) { throw 'Lifecycle test failed' }

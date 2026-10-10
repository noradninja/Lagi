param([string]$DevShell='C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/Common7/Tools/Launch-VsDevShell.ps1')
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$source=Get-Content -Raw "$root/src/integration/lagi_vdp1_animated_quad_vita.cpp"
$start=$source.IndexOf('int lagiDrawParticleWithColorTable(')
$end=$source.IndexOf('int vdp1DrawQuadScaled(', $start)
if($start -lt 0 -or $end -lt 0){throw 'Adapter extraction boundary changed'}
$preamble=@'
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
using quadColor=std::array<std::uint16_t,4>;
struct sAnimatedQuad { unsigned char m7_currentFrame=0; };
struct sVec3_FP {};
struct sSaturnPtr { unsigned offset; sSaturnPtr operator+(unsigned n) const {return {offset+n};} };
static std::array<unsigned char,2048> memory;
static unsigned reads=0,calls=0;
static quadColor captured;
static sAnimatedQuad* expectedParticle;
static sVec3_FP* expectedPosition;
static unsigned short readSaturnU16(sSaturnPtr p) {
 assert(p.offset+1<memory.size()); ++reads;
 return (unsigned(memory[p.offset])<<8)|memory[p.offset+1];
}
static int drawProjectedParticleWithGouraud(sAnimatedQuad* p,sVec3_FP* v,const quadColor* c) {
 assert(p==expectedParticle && v==expectedPosition && c);captured=*c;++calls;return 17;
}
'@
$driver=@'
int main() {
 for(unsigned i=0;i<memory.size();++i) memory[i]=(i*37+11)&255;
 sAnimatedQuad particle; sVec3_FP position;
 expectedParticle=&particle; expectedPosition=&position;
 unsigned cases=0;
 for(unsigned stride: {0u,4u,8u}) for(unsigned frame=0;frame<128;++frame) {
  particle.m7_currentFrame=frame;
  reads=0; calls=0;
  assert(lagiDrawParticleWithColorTable(&particle,&position,{16},stride)==17);
  assert(reads==4 && calls==1);
  for(unsigned corner=0;corner<4;++corner) {
   unsigned a=16+frame*stride+corner*2;
   assert(captured[corner]==((unsigned(memory[a])<<8)|memory[a+1]));
  }
  ++cases;
 }
 reads=0; calls=0;
 assert(lagiDrawParticleWithColorTable(nullptr,&position,{16},4)==0);
 assert(reads==0 && calls==0);
 std::printf("PASS: %u actual adapter color windows, endian order, forwarding and null particle\n",cases);
}
'@
$directory=Join-Path $root 'build/host-particle-color'
New-Item -ItemType Directory -Force $directory | Out-Null
$cpp=Join-Path $directory 'adapter.cpp'
$exe=Join-Path $directory 'adapter.exe'
[IO.File]::WriteAllText($cpp,$preamble+"`n"+$source.Substring($start,$end-$start)+"`n"+$driver)
& $DevShell -Arch amd64 -HostArch amd64 -SkipAutomaticLocation | Out-Null
& cl.exe /nologo /std:c++17 /EHsc /O2 "/Fe:$exe" "/Fo:$directory/" $cpp
if($LASTEXITCODE -ne 0){throw 'Adapter host compilation failed'}
& $exe
if($LASTEXITCODE -ne 0){throw 'Adapter host test failed'}

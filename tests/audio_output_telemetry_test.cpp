#include "lagi/audio_output_telemetry.h"
#include <cassert>
#include <cstdio>
namespace lagi::audio { OutputQueueTelemetry outputQueueTelemetry; }
int main() {
  lagi::audio::OutputQueueTelemetry t;
  auto initial=t.take_interval();
  assert(initial.blocks==0 && initial.minQueued==0);
  t.record(2048,1024);
  t.record(256,1024);
  t.record(0,1024);
  t.record(100,0);
  auto a=t.take_interval();
  assert(a.blocks==3 && a.shortBlocks==2 && a.emptyBlocks==1);
  assert(a.zeroFrames==1792 && a.maxMissing==1024);
  assert(a.minQueued==0 && a.maxQueued==2048);
  auto b=t.take_interval();
  assert(b.blocks==0 && b.zeroFrames==0 && b.maxQueued==0);
  t.record(1024,1024);
  t.clear();
  assert(t.take_interval().blocks==0);
  std::puts("PASS output PCM zero-fill counting and window reset");
}

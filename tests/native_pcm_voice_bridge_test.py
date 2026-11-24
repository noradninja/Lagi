from pathlib import Path
import subprocess,tempfile
r=Path(__file__).resolve().parents[1]
s=(r/'build-vita/azel_upstream/town_townCutscene.cpp').read_text()
a=s.index('static void lagiUpdateRawTownPCM(sStreamingFile* stream)');b=s.index('void sCutsceneTask::cutsceneDrawSub0',a);raw=s[a:b]
a=s.index('void drawPCMDebugDisplay(');b=s.index('// 06036224',a);draw=s[a:b]
a=s.index('sStreamingFile* openPCMFileForStreaming(');b=s.index('static void lagiUpdateRawTownPCM(sStreamingFile*);',a)
assert 'fopen(' not in s[a:b] and 'gSoloud' not in s[a:b] and 'openFileForStreaming' in s[a:b]
v=(r/'build-vita/azel_upstream/audio_pcmVoiceTask.cpp').read_text();a=v.index('void sPCMVoiceTask::Draw(');b=v.index('// 060772e4',a);voice=v[a:b]
f=r"""
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstddef>
#include <vector>
#include <cstdio>
using u32=uint32_t;using u64=uint64_t;using s32=int32_t;using u8=uint8_t;
struct Metadata{u32 m10_totalFrames=0;};
struct sStreamingFile{struct{int m0=2;u32 m24_currentPositionInFile=0,m28=0,m60=22050,m84_frameIndex=0;const u8* m34_pBufferRead=nullptr;const u8* m38=nullptr;const u8* m3C=nullptr;}m28;struct{u32 m18_fileSize=0;}m198;Metadata* m194;};
struct Task{bool finished=false;void markFinished(){finished=true;}};
struct sPCMVoiceTask{sStreamingFile* m0;int m1D8_state=0;Task task;Task* getTask(){return &task;}static void Draw(sPCMVoiceTask*);};
namespace lagi::platform::logging{template<class...T>void writef(const char*,T...) {}}
u32 queued=0,consumed=0;int result=0,submits=0;const u8* base;u32 ringSize;
void updateStreamingFileRead(sStreamingFile*){}
int lagiSubmitCutscenePCM(sStreamingFile* stream,const u8* read,u32 bytes,u32 rate,int channels,int bits,const u8* wrap,size_t contiguous){
 assert(read==stream->m28.m34_pBufferRead&&rate==stream->m28.m60&&channels==1&&bits==16&&wrap==base&&bytes<=0x4000&&!(bytes&1));
 assert(contiguous==std::min<size_t>(bytes,base+ringSize-read));++submits;if(!result)queued+=bytes/2;return result;
}
void executeCutsceneCommandsSub0Sub0(sStreamingFile* s,u32 bytes){consumed+=bytes;s->m28.m28+=bytes;s->m28.m34_pBufferRead=base+(s->m28.m34_pBufferRead-base+bytes)%ringSize;}
u32 lagiCutscenePCMQueued(sStreamingFile*){return queued;}
void scriptFunction_60573d8Sub0(sStreamingFile*){}
s32 scriptFunction_605861eSub0Sub0(sStreamingFile* s){return s?s->m28.m0:-1;}
// RAW
// DRAW
// VOICE
int main(){
 std::vector<u8> ring(32768);base=ring.data();ringSize=ring.size();
 for(u32 rate:{18000u,22050u})for(u32 start:{0u,32760u})for(u32 bytes:{2u,16384u,30000u}){
  Metadata metadata{u32(u64(bytes/2)*60/rate)};sStreamingFile stream;stream.m194=&metadata;stream.m198.m18_fileSize=bytes;stream.m28.m60=rate;stream.m28.m24_currentPositionInFile=bytes;stream.m28.m34_pBufferRead=base+start;stream.m28.m38=base+ringSize;stream.m28.m3C=base;
  sPCMVoiceTask task{&stream};queued=consumed=submits=0;result=-1;
  task.Draw(&task);assert(!consumed&&!queued&&!task.task.finished&&stream.m28.m34_pBufferRead==base+start);
  result=0;for(int tick=0;tick<4&&consumed<bytes;++tick)task.Draw(&task);
  assert(consumed==bytes&&queued==bytes/2&&stream.m28.m0!=5&&!task.task.finished&&stream.m28.m84_frameIndex==0);
  if(queued>1){queued/=2;task.Draw(&task);assert(stream.m28.m84_frameIndex==u32(u64(bytes/2-queued)*60/rate)&&!task.task.finished);}
  queued=0;task.Draw(&task);assert(stream.m28.m0==5&&stream.m28.m84_frameIndex==metadata.m10_totalFrames&&task.task.finished);
 }
 // Rejected payloads set an error instead of consuming or reporting completion.
 Metadata metadata;sStreamingFile stream;stream.m194=&metadata;stream.m198.m18_fileSize=2;stream.m28.m24_currentPositionInFile=2;stream.m28.m34_pBufferRead=base;stream.m28.m38=base+ringSize;stream.m28.m3C=base;queued=consumed=0;result=-2;drawPCMDebugDisplay(&stream);assert(stream.m28.m0==-1&&!consumed);
 puts("PASS generated native voice Draw uses shared PCM ring: busy retry, wrap, mono BE16 rates, queue drain, task completion and invalid payload");
}
""".replace('// RAW',raw).replace('// DRAW',draw).replace('// VOICE',voice)
with tempfile.TemporaryDirectory(prefix='lagi-voice-bridge-') as d:
 p=Path(d);(p/'test.cpp').write_text(f)
 subprocess.run(['cl','/nologo','/EHsc','/std:c++20',str(p/'test.cpp'),'/Fe:'+str(p/'test.exe'),'/Fo:'+str(p/'test.obj')],check=True)
 subprocess.run([str(p/'test.exe')],check=True)

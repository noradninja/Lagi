#include "lagi/cdda_stream.h"
#include <cassert>
#include <cstdio>
#include <string>
int main(int argc,char** argv){
 assert(argc>=2);lagi::CddaTrack track;
 const std::string cue="FILE \"all.bin\" BINARY\n TRACK 01 MODE1/2352\n INDEX 01 00:00:00\n TRACK 02 AUDIO\n PREGAP 00:02:00\n INDEX 01 01:00:00\n TRACK 03 AUDIO\n INDEX 00 01:10:00\n INDEX 01 01:12:00\n TRACK 04 AUDIO\n INDEX 00 02:00:00\n INDEX 01 02:02:00\n";
 assert(lagi::cdda_track_from_cue(cue,3,track)&&track.firstByte==5400ull*2352&&track.endByte==9000ull*2352);
 assert(lagi::cdda_track_from_cue(cue,2,track)&&track.firstByte==4500ull*2352&&track.endByte==5250ull*2352);
 assert(!lagi::cdda_track_from_cue(cue,1,track));
 assert(!lagi::cdda_track_from_cue(cue,5,track));
 const std::string split="FILE \"data.bin\" BINARY\n TRACK 01 MODE1/2352\n INDEX 01 00:00:00\nFILE \"second.bin\" BINARY\n TRACK 02 AUDIO\n INDEX 01 00:00:00\nFILE \"title.bin\" BINARY\n TRACK 03 AUDIO\n PREGAP 00:02:00\n INDEX 01 00:00:00\n";
 assert(lagi::cdda_track_from_cue(split,3,track)&&track.file=="title.bin"&&track.firstByte==0&&track.endByte==0);
 assert(!lagi::cdda_track_from_cue("FILE \"a\" WAVE\n TRACK 03 AUDIO\n INDEX 01 00:00:00",3,track));
 assert(!lagi::cdda_track_from_cue("FILE \"a\" BINARY\n TRACK 03 AUDIO\n INDEX 01 00:60:00",3,track));
 assert(!lagi::cdda_track_from_cue("FILE \"a\" BINARY\n TRACK 03 AUDIO",3,track));
 assert(!lagi::cdda_track_from_cue("FILE \"a\" BINARY\n TRACK 01 MODE1/2048\n INDEX 01 00:00:00\n TRACK 03 AUDIO\n INDEX 01 01:00:00",3,track));
 FILE* f=std::fopen(argv[1],"wb");assert(f);
 // Two ignored frames, then (32767,-32768), (1,-1), then ignored data.
 const unsigned char bytes[]={99,99,99,99,88,88,88,88,255,127,0,128,1,0,255,255,77,77,77,77};
 assert(std::fwrite(bytes,1,sizeof(bytes),f)==sizeof(bytes));std::fclose(f);
 track={argv[1],8,16};lagi::CddaStream stream;
 assert(stream.start(track,false));std::int16_t out[8]={100,-100,0,0,123,123,123,123};stream.mix(out,4);
 assert(out[0]==32767&&out[1]==-32768&&out[2]==1&&out[3]==-1&&out[4]==123&&!stream.active());
 assert(stream.start(track,true));std::int16_t loop[12]{};stream.mix(loop,6);
 for(unsigned i=0;i<6;++i)assert(loop[i*2]==(i%2?1:32767)&&loop[i*2+1]==(i%2?-1:-32768));
 stream.fade(2);std::int16_t faded[8]{};stream.mix(faded,4);assert(!stream.active()&&faded[4]==0&&faded[6]==0);
 assert(stream.start(track,true));stream.stop();assert(!stream.active());
 track.endByte=24;assert(!stream.start(track,false));
 std::remove(argv[1]);
 if(argc==4){
  FILE* cueFile=std::fopen(argv[2],"rb");assert(cueFile);std::string text;char buffer[1024];std::size_t got;
  while((got=std::fread(buffer,1,sizeof(buffer),cueFile)))text.append(buffer,got);std::fclose(cueFile);
  assert(lagi::cdda_track_from_cue(text,3,track));track.file=argv[3];
  FILE* bin=std::fopen(argv[3],"rb");assert(bin);assert(std::fseek(bin,0,SEEK_END)==0);
  const long length=std::ftell(bin);assert(length>0);std::fclose(bin);
  if(!track.endByte)track.endByte=static_cast<std::uint64_t>(length);
  assert(track.firstByte<track.endByte&&track.endByte<=static_cast<std::uint64_t>(length));
  assert((track.endByte-track.firstByte)%2352==0);assert(stream.start(track,true));
  std::int16_t original[1176]{};stream.mix(original,588);assert(stream.active());stream.stop();
  std::printf("PASS original Disc CUE track 3 bounds: start=%llu end=%llu frames=%llu; bounded first-sector read, not listening proof\n",
   static_cast<unsigned long long>(track.firstByte),static_cast<unsigned long long>(track.endByte),
   static_cast<unsigned long long>((track.endByte-track.firstByte)/4));
 }
 puts("PASS production CUE ranges, split files, stored/synthetic pregaps, LE stereo, bounded reads, clipping, looping, fade, stop and truncation; not Vita listening evidence");
}

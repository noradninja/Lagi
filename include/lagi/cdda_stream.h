#pragma once
#include "lagi/cdda_cue.h"
#include <algorithm>
#include <array>
#include <climits>
namespace lagi {
// Audio-worker-owned, bounded streaming of raw 44.1kHz stereo LE PCM.
// BIN byte order matches the CD lane used by Yabause's SCSP mixer.
class CddaStream {
 FILE* file_=nullptr;CddaTrack track_;bool loop_=false;
 std::uint64_t next_=0;std::size_t read_=0,size_=0;
 std::array<unsigned char,2352*16> buffer_{};
 unsigned fade_=0,fadeTotal_=0;
 bool refill(){
  if(next_==track_.endByte){if(!loop_){stop();return false;}next_=track_.firstByte;}
  const auto bytes=static_cast<std::size_t>(std::min<std::uint64_t>(buffer_.size(),track_.endByte-next_));
  if(next_>LONG_MAX||std::fseek(file_,static_cast<long>(next_),SEEK_SET)!=0){stop();return false;}
  size_=std::fread(buffer_.data(),1,bytes,file_);read_=0;
  if(size_!=bytes||size_%4){stop();return false;}next_+=size_;return size_!=0;
 }
public:
 ~CddaStream(){stop();}
 CddaStream()=default;CddaStream(const CddaStream&)=delete;CddaStream& operator=(const CddaStream&)=delete;
 bool active()const{return file_!=nullptr;}
 void stop(){if(file_)std::fclose(file_);file_=nullptr;read_=size_=0;fade_=fadeTotal_=0;}
 void fade(unsigned frames){if(!frames)stop();else fade_=fadeTotal_=frames;}
 bool start(const CddaTrack& track,bool loop){
  stop();if(track.firstByte>=track.endByte||track.endByte>LONG_MAX||(track.endByte-track.firstByte)%4)return false;
  file_=std::fopen(track.file.c_str(),"rb");if(!file_)return false;
  if(std::fseek(file_,0,SEEK_END)!=0||std::ftell(file_)<static_cast<long>(track.endByte)){stop();return false;}
  track_=track;loop_=loop;next_=track.firstByte;return true;
 }
 void mix(std::int16_t* output,std::size_t frames){
  for(std::size_t i=0;i<frames&&active();++i){
   if(read_==size_&&!refill())break;
   for(unsigned channel=0;channel<2;++channel){
    const unsigned offset=static_cast<unsigned>(read_)+channel*2;
    const auto bits=static_cast<std::uint16_t>(buffer_[offset]|(unsigned(buffer_[offset+1])<<8));
    int sample=bits<0x8000?int(bits):int(bits)-65536;
    if(fadeTotal_)sample=static_cast<int>(std::int64_t(sample)*fade_/fadeTotal_);
    output[i*2+channel]=static_cast<std::int16_t>(std::clamp(int(output[i*2+channel])+sample,-32768,32767));
   }
   read_+=4;if(fadeTotal_&&!--fade_)stop();
  }
 }
};
}

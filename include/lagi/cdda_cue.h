#pragma once
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>
namespace lagi {
struct CddaTrack { std::string file; std::uint64_t firstByte=0,endByte=0; };
// CUE indices are file-relative stored sectors. PREGAP is synthetic silence,
// not a byte offset. INDEX 00 belongs to the next track's stored pregap.
inline bool cdda_track_from_cue(const std::string& text,unsigned wanted,CddaTrack& out) {
 struct Track { unsigned number=0;std::string file;bool audio=false,binary=false,raw=false,has01=false,has00=false;std::uint64_t index01=0,index00=0; };
 std::vector<Track> tracks;std::string file,line;bool binary=false;
 std::size_t cursor=0;
 while(cursor<text.size()) {
  const auto end=text.find('\n',cursor);
  line=text.substr(cursor,end==std::string::npos?end:end-cursor);
  cursor=end==std::string::npos?text.size():end+1;
  const auto start=line.find_first_not_of(" \t\r");if(start==std::string::npos)continue;line.erase(0,start);
  if(line.rfind("FILE ",0)==0){
   const auto a=line.find('"'),b=a==std::string::npos?a:line.find('"',a+1);
   if(a==std::string::npos||b==std::string::npos)return false;
   file=line.substr(a+1,b-a-1);binary=line.substr(b+1).find("BINARY")!=std::string::npos;
  }else if(line.rfind("TRACK ",0)==0){
   unsigned number;char type[32]{};if(std::sscanf(line.c_str(),"TRACK %u %31s",&number,type)!=2)return false;
   Track track;track.number=number;track.file=file;track.binary=binary;track.audio=std::string(type)=="AUDIO";
   track.raw=track.audio||std::string(type)=="MODE1/2352"||std::string(type)=="MODE2/2352";tracks.push_back(track);
  }else if(line.rfind("INDEX ",0)==0&&!tracks.empty()){
   unsigned index,m,s,f;if(std::sscanf(line.c_str(),"INDEX %u %u:%u:%u",&index,&m,&s,&f)!=4||s>=60||f>=75)return false;
   const auto sector=(std::uint64_t(m)*60+s)*75+f;
   if(index==1){tracks.back().has01=true;tracks.back().index01=sector;}
   if(index==0){tracks.back().has00=true;tracks.back().index00=sector;}
  }
 }
 for(std::size_t i=0;i<tracks.size();++i){const auto& track=tracks[i];if(track.number!=wanted)continue;
  if(!track.audio||!track.binary||!track.has01||track.file.empty())return false;
  if(track.has00&&track.index00>track.index01)return false;
  // Mixed stored sector sizes in one FILE need a different offset model.
  for(const auto& other:tracks)if(other.file==track.file&&!other.raw)return false;
  out={track.file,track.index01*2352,0};
  if(i+1<tracks.size()&&tracks[i+1].file==track.file){const auto& next=tracks[i+1];
   if(!next.has01)return false;out.endByte=(next.has00?next.index00:next.index01)*2352;
   if(out.endByte<=out.firstByte)return false;
  }
  return true;
 }
 return false;
}
}

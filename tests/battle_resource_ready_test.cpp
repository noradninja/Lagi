#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>
using u8=std::uint8_t;using u16=std::uint16_t;
struct sSaturnPtr{const char* p=nullptr;bool isNull()const{return !p;}};
static std::string readSaturnString(sSaturnPtr p){return p.p;}
bool isSoundFileLoaded(sSaturnPtr);
static bool readSuccess=true;
static int selectedDisc=1;
static unsigned reads=0;
namespace lagi::disc {
 int selected_disc(){return selectedDisc;}
 bool read_file(const char* filename,std::vector<u8>& data){
  ++reads;assert(!isSoundFileLoaded({filename})); // Pending until copy finishes.
  if(!readSuccess)return false;
  data={11,22,33};return true;
 }
}
namespace lagi::platform::logging{template<class... T>void writef(const char*,T...) {}}
static struct{int m0=1;}fileInfoStruct;
struct s_fileBundle {
 u8* raw;u16 patch=0;
 explicit s_fileBundle(u8* p):raw(p){}
 void setPatchFilePointers(u16 p){patch=p;}
 ~s_fileBundle(){delete[] raw;}
};
#include "service.inc"
int main(){
 u8 destination[3]={};
 assert(!isSoundFileLoaded({"TEST.SCB"}) && reads==0);
 assert(loadFile("TEST.SCB",destination,0)==0 && destination[2]==33);
 assert(isSoundFileLoaded({"test.scb"}) && reads==1);
 readSuccess=false;
 assert(loadFile("TEST.SCB",destination,0)<0 && !isSoundFileLoaded({"TEST.SCB"}));
 readSuccess=true;
 assert(loadFile2("TEST.SCB",destination,0)==0 && isSoundFileLoaded({"TEST.SCB"}));
 assert(loadFile("TEST.SCB",static_cast<u8*>(nullptr),0)<0 && !isSoundFileLoaded({"TEST.SCB"}));
 s_fileBundle* bundle=nullptr;
 assert(loadFile("TEST.PNB",&bundle,0x123)==0 && bundle && bundle->raw[0]==11 && bundle->patch==0x123);
 assert(isSoundFileLoaded({"TEST.PNB"}));delete bundle;
 bundle=nullptr;readSuccess=false;
 assert(loadFile("TEST.PNB",&bundle,0)<0 && !bundle && !isSoundFileLoaded({"TEST.PNB"}));
 readSuccess=true;
 selectedDisc=2;assert(!isSoundFileLoaded({"TEST.PNB"}));
 assert(loadFile("TEST.SCB",destination,0)==0 && isSoundFileLoaded({"TEST.SCB"}));
 selectedDisc=1;assert(!isSoundFileLoaded({"TEST.SCB"}));
 assert(!isSoundFileLoaded({}));
 assert(loadFile(nullptr,destination,0)<0);
 std::puts("PASS: actual file service readiness follows successful copy/bundle publication, rejects failed/pending/unknown/null loads, handles aliases and resets on disc change; mocked byte I/O");
}

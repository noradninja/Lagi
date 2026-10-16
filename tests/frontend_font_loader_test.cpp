#include "lagi/frontend_font.h"
#include "lagi/disc_image.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
unsigned char defaultFontPalettes[512]{};
static std::string root,mode;
static std::vector<std::uint8_t> synthetic;
static unsigned probes=0,logs=0;
namespace lagi::platform::filesystem {const char* data_root(){return root.c_str();}}
namespace lagi::platform::logging {void writef(const char*,...){++logs;}}
namespace lagi::disc {
bool read_file_from_disc(int number,const char* name,std::vector<std::uint8_t>& out,std::size_t max){
 ++probes;assert(!std::strcmp(name,"ASCII.CGZ") && max==65536);
 if(mode=="disc" && number==2){out=synthetic;return true;}
 if(mode=="invalid"){out={0,0};return true;}
 out.clear();return false;
}}
int main(int argc,char** argv){assert(argc==3);mode=argv[1];root=argv[2];
 for(unsigned i=1;i<16;++i){defaultFontPalettes[384+i*2]=0x7F;defaultFontPalettes[385+i*2]=0xFF;}
 synthetic.push_back(0);unsigned used=0;std::size_t control=0;
 auto bit=[&](unsigned b){if(used==8){control=synthetic.size();synthetic.push_back(0);used=0;}synthetic[control]|=b<<used++;};
 for(unsigned i=0;i<(0x63+95*2)*32;++i){bit(1);synthetic.push_back(0x11);}
 bit(0);bit(1);synthetic.push_back(0);synthetic.push_back(0);
 // Synthetic pixels exercise loader ownership only; real-disc glyph fidelity
 // is tested separately. No copyrighted fixture is embedded here.
 if(mode=="loose") {FILE* file=nullptr;assert(!fopen_s(&file,(root+"/ASCII.CGZ").c_str(),"wb"));assert(std::fwrite(synthetic.data(),1,synthetic.size(),file)==synthetic.size());std::fclose(file);}
 const bool expected=mode=="loose" || mode=="disc";
 assert(lagi::frontend::initializeFont()==expected && lagi::frontend::font().ready()==expected);
 assert(probes==(mode=="loose"?0u:mode=="disc"?2u:4u));
 const auto count=probes;assert(lagi::frontend::initializeFont()==expected && probes==count && logs==1);
 if(expected)assert(lagi::frontend::font().measure("Loading...")==80);
 std::printf("PASS: production frontend font initialization/cache/fallback mode=%s; no Azel text initialization\n",mode.c_str());
}

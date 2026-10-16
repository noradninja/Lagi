#include "lagi/frontend_font.h"
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <string>
#include <vector>
using u8=std::uint8_t;using u16=std::uint16_t;using s16=std::int16_t;
#include "native_font.inc"
using namespace lagi::frontend;
namespace lagi::frontend {static LatinFont testFont;const LatinFont& font(){return testFont;}}
static constexpr int kWidth=960,kHeight=544,kPitch=960;
static constexpr unsigned kFrameBytes=kWidth*kHeight*4;
static std::array<std::uint32_t,kWidth*kHeight> frame0,frame1;
static std::uint32_t* g_frameBuffer[2]={frame0.data(),frame1.data()};
static int g_drawBuffer=0,fallbackX=0,fallbackY=0,fallbackCalls=0;
static void drawTextSmall(int x,int y,const char*,std::uint32_t){fallbackX=x;fallbackY=y;++fallbackCalls;}
struct LauncherSprite {unsigned width=0,height=0;std::vector<std::uint32_t> rgba;};
static LauncherSprite g_launcherSprites[14];
#include "frontend_text.inc"
#include "frontend_rect.inc"
#include "frontend_launcher.inc"
struct Encoder {
 std::vector<u8> data{0};unsigned used=0;std::size_t control=0;
 void bit(unsigned b){if(used==8){control=data.size();data.push_back(0);used=0;}data[control]|=b<<used++;}
 void byte(unsigned b){data.push_back(static_cast<u8>(b));}
 void literal(unsigned b){bit(1);byte(b);}
 void end(){bit(0);bit(1);byte(0);byte(0);}
};
static std::vector<u8> extent(FILE* disc,unsigned lba,unsigned bytes){
 std::vector<u8> out(bytes);
 for(unsigned at=0;at<bytes;at+=2048){const auto count=std::min(2048u,bytes-at);assert(!_fseeki64(disc,(std::int64_t(lba)+at/2048)*2352+16,SEEK_SET));assert(std::fread(out.data()+at,1,count,disc)==count);}
 return out;
}
static unsigned le32(const u8* p){return unsigned(p[0])|(unsigned(p[1])<<8)|(unsigned(p[2])<<16)|(unsigned(p[3])<<24);}
static std::vector<u8> readFont(const char* path){
 FILE* disc=nullptr;assert(!fopen_s(&disc,path,"rb") && disc);
 auto pvd=extent(disc,16,2048);assert(!std::memcmp(pvd.data()+1,"CD001",5));
 auto directory=extent(disc,le32(pvd.data()+158),le32(pvd.data()+166));
 for(unsigned at=0;at<directory.size();){
  const auto count=directory[at];if(!count){at=(at/2048+1)*2048;continue;}
  assert(count>=34 && at+count<=directory.size());const auto* rec=directory.data()+at;
  const unsigned nameBytes=rec[32];assert(33+nameBytes<=count);
  if(nameBytes>=9 && !std::memcmp(rec+33,"ASCII.CGZ",9)){
   auto font=extent(disc,le32(rec+2),le32(rec+10));std::fclose(disc);return font;
  }at+=count;
 }std::fclose(disc);assert(false);return {};
}
int main(int argc,char** argv){
 assert(argc==4);
 Encoder literal;for(unsigned i=0;i<19;++i)literal.literal('A'+i);literal.end();
 std::array<u8,128> output{};
 auto r=unpackFont(literal.data.data(),literal.data.size(),output.data(),output.size());assert(r.ok && r.written==19);
 for(unsigned i=0;i<19;++i)assert(output[i]=='A'+i);
 Encoder shortCopy;shortCopy.literal('X');shortCopy.bit(0);shortCopy.bit(0);shortCopy.bit(0);shortCopy.bit(1);shortCopy.byte(255);shortCopy.end();
 r=unpackFont(shortCopy.data.data(),shortCopy.data.size(),output.data(),output.size());assert(r.ok && r.written==4 && !std::memcmp(output.data(),"XXXX",4));
 Encoder longCopy;longCopy.literal('Z');longCopy.bit(0);longCopy.bit(1);longCopy.byte(0xF8);longCopy.byte(0xFF);longCopy.byte(5);longCopy.end();
 r=unpackFont(longCopy.data.data(),longCopy.data.size(),output.data(),output.size());assert(r.ok && r.written==7);
 for(unsigned i=0;i<7;++i)assert(output[i]=='Z');
 for(unsigned count=2;count<=5;++count){Encoder stream;stream.literal('Q');stream.bit(0);stream.bit(0);stream.bit((count-2)>>1);stream.bit((count-2)&1);stream.byte(255);stream.end();auto copy=unpackFont(stream.data.data(),stream.data.size(),output.data(),output.size());assert(copy.ok && copy.written==count+1);}
 for(unsigned count=3;count<=9;++count){Encoder stream;stream.literal('R');stream.bit(0);stream.bit(1);stream.byte(0xF8|(count-2));stream.byte(0xFF);stream.end();auto copy=unpackFont(stream.data.data(),stream.data.size(),output.data(),output.size());assert(copy.ok && copy.written==count+1);}
 assert(!unpackFont(longCopy.data.data(),longCopy.data.size(),output.data(),3).ok);
 const u8 invalidBackref[]={0,0xFF};assert(!unpackFont(invalidBackref,sizeof(invalidBackref),output.data(),output.size()).ok);
 assert(!unpackFont(nullptr,5,output.data(),output.size()).ok);
 auto compressed=readFont(argv[1]);
 std::vector<u8> decoded(LatinFont::maximumDecodedBytes),reference(0x80000);
 const auto real=unpackFont(compressed.data(),compressed.size(),decoded.data(),decoded.size());assert(real.ok);
 unpackGraphicsToVDP2(compressed.data(),reference.data());
 assert(!std::memcmp(decoded.data(),reference.data(),decoded.size()));
 for(std::size_t n=0;n<real.consumed;++n)assert(!unpackFont(compressed.data(),n,decoded.data(),decoded.size()).ok);
 auto& font=lagi::frontend::testFont;assert(font.load(compressed.data(),compressed.size(),defaultFontPalettes+12*32));
 assert(font.measure("DISC 1")==48 && font.measure("Loading...")==80 && font.measure("AA\nA")==16);
 unsigned compared=0,ink=0;
 for(unsigned c=32;c<127;++c)for(unsigned y=0;y<16;++y)for(unsigned x=0;x<8;++x){
  const auto tile=0x63+2*(c-32)+y/8;const auto byte=reference[tile*32+(y&7)*4+x/2];
  const unsigned expected=x&1?byte&15:byte>>4;assert(font.pixel(c,x,y)==expected);ink+=expected!=0;++compared;
 }assert(ink>0);
 for(unsigned y=0;y<16;++y)for(unsigned x=0;x<8;++x)assert(font.pixel(' ',x,y)==0);
 std::vector<std::uint32_t> surface(352*144,0xFF211811);
 const char* lines[]={"ABCDEFGHIJKLMNOPQRSTUVWXYZ","abcdefghijklmnopqrstuvwxyz","0123456789 !?:/.-","DISC 1   CONTINUE FROM DISC 1","Loading..."};
 for(unsigned i=0;i<5;++i)font.draw(surface.data(),352,144,352,8,8+i*26,lines[i]);
 FILE* preview=nullptr;assert(!fopen_s(&preview,argv[2],"wb"));std::fprintf(preview,"P6\n704 288\n255\n");
 for(unsigned y=0;y<288;++y)for(unsigned x=0;x<704;++x){const auto color=surface[(y/2)*352+x/2];const u8 rgb[]={u8(color),u8(color>>8),u8(color>>16)};std::fwrite(rgb,1,3,preview);}std::fclose(preview);
 std::array<std::uint32_t,36> clipped;clipped.fill(0x12345678);font.draw(clipped.data()+6,4,4,6,-3,-7,"ABC");
 for(unsigned i=0;i<6;++i)assert(clipped[i]==0x12345678);
 for(unsigned i=30;i<36;++i)assert(clipped[i]==0x12345678);
 for(unsigned row=0;row<4;++row)for(unsigned x=4;x<6;++x)assert(clipped[6+row*6+x]==0x12345678);
 std::array<std::uint32_t,128> question{},unknown{};
 font.draw(question.data(),8,16,8,0,0,"?");font.draw(unknown.data(),8,16,8,0,0,"\xFF");assert(question==unknown);
 const char* assets[]={"background","outer_frame","radial","gold","ivory","missing","highlight","row","info","legend","disc_1","disc_2","disc_3","disc_4"};
 for(unsigned i=0;i<14;++i){
  FILE* asset=nullptr;assert(!fopen_s(&asset,(std::string(argv[3])+"/"+assets[i]+".rgba").c_str(),"rb"));
  std::uint32_t header[2];assert(std::fread(header,sizeof(header),1,asset)==1);
  auto& sprite=g_launcherSprites[i];sprite.width=header[0];sprite.height=header[1];sprite.rgba.resize(sprite.width*sprite.height);
  assert(std::fread(sprite.rgba.data(),4,sprite.rgba.size(),asset)==sprite.rgba.size());std::fclose(asset);
 }
 auto saveFrame=[&](const std::string& path){FILE* out=nullptr;assert(!fopen_s(&out,path.c_str(),"wb"));std::fprintf(out,"P6\n960 544\n255\n");for(auto pixel:frame0){const u8 rgb[]={u8(pixel),u8(pixel>>8),u8(pixel>>16)};std::fwrite(rgb,1,3,out);}std::fclose(out);};
 const bool available[4]={true,true,true,true};
 for(int selected=0;selected<4;++selected){
  g_drawBuffer=0;launcherTextured(available,selected);
  if(selected==0)saveFrame(std::string(argv[2])+".selector.ppm");
  frame1.fill(0xDEADBEEF);g_drawBuffer=1;launcherTextured(available,selected);assert(frame0==frame1);
 }
 g_drawBuffer=0;frame0.fill(0xFF000000);drawFrontendLoading();saveFrame(std::string(argv[2])+".loading.ppm");
 for(int y=0;y<kHeight;++y)for(int x=0;x<kWidth;++x)if(x<864 || x>=944 || y<514 || y>=530)assert(frame0[y*kPitch+x]==0xFF000000);
 assert(fallbackCalls==0);
 assert(!font.load(nullptr,0,defaultFontPalettes+12*32) && !font.ready());
 drawFrontendLoading();assert(fallbackCalls==1 && fallbackX==884 && fallbackY==523 && measureFrontendText("Loading...")==60);
 std::printf("PASS: native decoder differential (%zu compressed, %zu decoded, %zu consumed), %u ASCII pixels, metrics, clipping and %zu truncated streams; preview is local only\n",compressed.size(),real.written,real.consumed,compared,real.consumed);
}

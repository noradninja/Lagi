#include "lagi/azel_map_menu.h"
#include <cassert>
#include <cstdio>
#include <string>
#include <vector>
static std::array<unsigned char,0x21084> common;
struct Common {unsigned char* m_data=common.data();unsigned m_base=0x200000,m_dataSize=unsigned(common.size());} commonObject;
Common* gCommonFile=&commonObject;
struct {struct {unsigned short fontIndex=9;} m40AC;} graphicEngineStatus;
struct {int m0=0,m4_cursorX=0,m8_cursorY=0,mC_X=0,m10_Y=0;} vdp2StringContext;
static std::vector<int> calls;
struct Printed {std::string text;int x,y,style;};
static std::vector<Printed> printed;
void setupVDP2StringRendering(int x,int y,int width,int height){
    assert(x==8 && y==0x38 && width==0x1C && height==4);
    vdp2StringContext.mC_X=vdp2StringContext.m4_cursorX=x;
    vdp2StringContext.m10_Y=vdp2StringContext.m8_cursorY=y;calls.push_back(1);
}
void clearVdp2TextArea(){calls.push_back(2);}
int setActiveFont(unsigned short index){assert(index==9);calls.push_back(3);return 4;}
void drawObjectName(const char* text){
    assert(text);printed.push_back({text,vdp2StringContext.m4_cursorX,vdp2StringContext.m8_cursorY,vdp2StringContext.m0});
    vdp2StringContext.m4_cursorX+=3;++vdp2StringContext.m8_cursorY;calls.push_back(4);
}
#include "map_message_production.inc"
static void reset(){calls.clear();printed.clear();vdp2StringContext.m0=0x77;}
int main(int argc,char** argv)
{
    std::memcpy(common.data()+0xD9B9,"FIELD",6);std::memcpy(common.data()+0xD9CF,"NONE",5);
    std::memcpy(common.data()+0xD9DE,"WORLD",6);std::memcpy(common.data()+0xD9EE,"BLOCKED",8);
    reset();assert(lagiAzelPresentMapMessage(0,{1,-1}));
    assert((calls==std::vector<int>{1,2,3,4}) && printed.size()==1 && printed[0].text=="FIELD");
    assert(printed[0].x==8 && printed[0].y==56 && printed[0].style==0);
    assert(vdp2StringContext.m4_cursorX==8 && vdp2StringContext.m8_cursorY==58);
    reset();assert(lagiAzelPresentMapMessage(0,{0,1}));
    assert((calls==std::vector<int>{1,2,3,4,4}) && printed.size()==2 && printed[1].text=="NONE");
    assert(printed[1].x==8 && printed[1].y==58 && printed[1].style==0);
    reset();assert(lagiAzelPresentMapMessage(1,{0,1}) && printed[0].text=="WORLD");
    assert(printed.size()==1 && vdp2StringContext.m4_cursorX==11 && vdp2StringContext.m8_cursorY==57);
    reset();assert(lagiAzelPresentMapMessage(1,{1,0}) && printed[0].text=="BLOCKED");
    reset();assert(lagiAzelPresentMapMessage(1,{1,-1}) && printed[0].text=="BLOCKED");
    reset();lagiAzelClearMapMessage();
    assert((calls==std::vector<int>{1,2,3}) && printed.empty() && vdp2StringContext.m0==0x77);
    reset();assert(!lagiAzelPresentMapMessage(-1,{1,1}) && printed.empty());
    reset();assert(!lagiAzelPresentMapMessage(2,{1,1}) && printed.empty());
    commonObject.m_dataSize=0xD9BC;reset();
    assert(!lagiAzelPresentMapMessage(0,{1,0}) && printed.empty());
    commonObject.m_dataSize=unsigned(common.size());
    commonObject.m_base=0x300000;reset();assert(!lagiAzelPresentMapMessage(0,{1,0}));commonObject.m_base=0x200000;
    gCommonFile=nullptr;reset();assert(!lagiAzelPresentMapMessage(1,{1,1}));gCommonFile=&commonObject;

    if(argc==2) {
        FILE* disc=nullptr;assert(!fopen_s(&disc,argv[1],"rb") && disc);
        constexpr unsigned bytes=134788;
        for(unsigned offset=0;offset<bytes;offset+=2048) {
            const auto count=std::min(2048u,bytes-offset);
            assert(!_fseeki64(disc,(182LL+offset/2048)*2352+16,SEEK_SET));
            assert(fread(common.data()+offset,1,count,disc)==count);
        }
        fclose(disc);commonObject.m_dataSize=bytes;
        reset();assert(lagiAzelPresentMapMessage(0,{1,-1}));
        assert(printed[0].text=="Map of your location.");
        reset();assert(lagiAzelPresentMapMessage(0,{0,1}));
        assert(printed[1].text=="No map exists.");
        reset();assert(lagiAzelPresentMapMessage(1,{1,1}));
        assert(printed[0].text=="Go to World Map");
        reset();assert(lagiAzelPresentMapMessage(1,{1,0}));
        assert(printed[0].text=="Cannot go to World Map");
        std::puts("PASS: all four message strings resolved from original Disc1 COMMON (read-only)");
    }
    std::puts("PASS: production native Map message rectangle/font/order, both choices, availability, cursor placement, clear and malformed COMMON guards (host mocks; not rendered text)");
}

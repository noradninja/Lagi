#include "lagi/azel_books_menu.h"
#include <vector>
#include <string>
#include <cstdio>

struct Registers { unsigned short mFA_PRINB = 0xABCF; } registers;
struct { Registers* m4_pendingVdp2Regs = &registers; unsigned m_isDirty = 0; } vdp2Controls;
struct { int m0 = -1; } vdp2StringContext;
struct Event { int type, a, b, c, d; std::string text; };
std::vector<Event> events;
void setupVDP2StringRendering(int x, int y, int w, int h) { events.push_back({0,x,y,w,h,{}}); }
void clearVdp2TextArea() { events.push_back({1}); }
void drawObjectName(const char* text) { events.push_back({2, vdp2StringContext.m0,0,0,0,text}); }
void printVdp2Number2(int value, int length) { events.push_back({3,value,length}); }
void printVdp2String2(const char* text) { events.push_back({4,0,0,0,0,text}); }
void setVdp2LayerScroll(unsigned layer, unsigned x, unsigned y) {
    events.push_back({5,static_cast<int>(layer),static_cast<int>(x),static_cast<int>(y)});
}
void scrollMenu() { events.push_back({6}); }
// Mechanically extracted actual service body, not a duplicated model.
#include "books_presentation_production.inc"

int main()
{
    std::array<std::uint8_t,512> data{};
    auto put32 = [&](unsigned offset, unsigned value) {
        for (int byte=3; byte>=0; --byte) { data[offset+byte]=value; value>>=8; }
    };
    put32(0,4); put32(4,480); put32(8,360); put32(12,400);
    data[17]=1; data[19]=2;
    put32(360,480); data[365]=1; data[367]=1;
    data[480]='T';
    for (unsigned nativeValue : {0u,1u,2u,3u,480u}) {
        put32(404,nativeValue);
        events.clear(); registers.mFA_PRINB=0xABCF; vdp2Controls.m_isDirty=0;
        LagiAzelBooksBodyPage body;
        if (!lagiAzelBooksPresentText(data.data(),data.size(),0,0,1,body)) return 1;
        if (body.nativeValue!=nativeValue || vdp2Controls.m_isDirty!=1 ||
            registers.mFA_PRINB!=(0xABC8 | ((nativeValue==1 || nativeValue==2)?5:0))) return 2;
        const std::array<std::array<int,4>,4> regions{{{7,36,30,2},{7,39,30,2},{7,42,6,2},{5,46,34,14}}};
        unsigned region=0, clears=0, names=0, numbers=0, scrolls=0, publish=0, separators=0;
        for (const auto& event:events) {
            if (event.type==0) {
                if (region>=4 || std::array<int,4>{event.a,event.b,event.c,event.d}!=regions[region++]) return 3;
            } else if (event.type==1) ++clears;
            else if (event.type==2) { if (event.text!="T" || event.a!=0) return 4; ++names; }
            else if (event.type==3) { if (event.a!=1 || event.b!=1) return 5; ++numbers; }
            else if (event.type==4) { if (event.text!="\x80\x82") return 6; ++separators; }
            else if (event.type==5) {
                if (event.a!=2 || event.b!=(nativeValue==1?-104:152) || event.c!=256) return 7;
                ++scrolls;
            } else if (event.type==6) ++publish;
        }
        const bool illustration=nativeValue==1 || nativeValue==2;
        if (region!=4 || clears!=4 || names!=(nativeValue>3?3u:2u) || numbers!=2 ||
            separators!=1 || scrolls!=unsigned(illustration) || publish!=unsigned(illustration)) return 8;
    }
    events.clear(); LagiAzelBooksBodyPage body;
    if (lagiAzelBooksPresentText(data.data(),16,0,0,1,body) || !events.empty()) return 9;
    puts("PASS: actual Books text service, all native illustration commands, priority preservation and invalid-resource rejection (host mocks, not Vita rendering)");
}

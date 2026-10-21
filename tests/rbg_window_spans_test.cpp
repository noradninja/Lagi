#include "../include/lagi/rbg_window_spans.h"
#include <cassert>
#include <cstdio>
#include <chrono>
static unsigned char raw[0x80000]{};
static void put16(unsigned address,unsigned value){
    raw[address&0x7ffff]=value&255;raw[(address+1)&0x7ffff]=value>>8;
}
// Independent pixel predicate matching the prior general/native path.
static bool oracle(const float* ctrl,int x,int y) {
    const unsigned control=unsigned(ctrl[4]),mask=unsigned(ctrl[7]);
    int accepted[2]{};
    for(unsigned w=0;w<2;++w) {
        if(!(control&(w?8:2))){accepted[w]=3;continue;}
        const unsigned base=8+w*4;
        int first=int(ctrl[base]),last=int(ctrl[base+2]);
        if(mask&(1<<w)) {
            const unsigned address=(unsigned(ctrl[5+w])+y*4)&0x7ffff;
            const unsigned start=raw[address]|(raw[(address+1)&0x7ffff]<<8);
            const unsigned end=raw[(address+2)&0x7ffff]|(raw[(address+3)&0x7ffff]<<8);
            first=(start>>1)&511;last=(end>>1)&511;
            if(end==0xffff){first=1;last=0;}
        }
        const bool inside=x>=first && x<=last && y>=int(ctrl[base+1]) && y<=int(ctrl[base+3]);
        accepted[w]=inside==((control&(w?4:1))!=0)?1:0;
    }
    const int a=accepted[0],b=accepted[1];
    return (a&2)?(b&1):(b&2)?(a&1):(control&0x80)?(a || b):(a && b);
}
int main() {
    unsigned cases=0;std::uint64_t pixels=0;
    auto start=std::chrono::steady_clock::now();
    for(unsigned scenario=0;scenario<16;++scenario) {
        float ctrl[16]{};ctrl[5]=float(0x7fffd);ctrl[6]=float(0x3c000);
        ctrl[8]=float((scenario*37)%512);ctrl[10]=float((scenario*67+175)%512);
        ctrl[9]=scenario&1?0:112;ctrl[11]=scenario&2?223:111;
        ctrl[12]=float((scenario*73)%512);ctrl[14]=float((scenario*17+351)%512);
        ctrl[13]=scenario&4?0:1;ctrl[15]=scenario&8?222:223;
        for(unsigned y=0;y<224;++y)for(unsigned w=0;w<2;++w) {
            const unsigned address=unsigned(ctrl[5+w])+y*4;
            put16(address,((scenario*41+y*7+w*100)%512)*2);
            put16(address+2,(y+scenario)%7==0?0xffff:((scenario*83+y*11+w*30)%512)*2);
        }
        for(unsigned mask=0;mask<4;++mask)for(unsigned bits=0;bits<(scenario?32u:256u);++bits) {
            ctrl[4]=float(scenario?((bits&15)|((bits&16)<<3)):bits);ctrl[7]=float(mask);
            std::array<unsigned char,352*224> owner;owner.fill(255);
            unsigned counts[2]{},area[2]{};
            assert(lagi::emitRbgWindowSpans(ctrl,raw,[&](unsigned p,int y,int first,int last){
                assert(p<2 && y>=0 && y<224 && first>=0 && last<352 && first<=last);
                ++counts[p];area[p]+=last-first+1;
                for(int x=first;x<=last;++x) {
                    auto& pixel=owner[y*352+x];assert(pixel==255);pixel=static_cast<unsigned char>(p);
                }
            }));
            assert(counts[0]<=lagi::RbgWindowMaxSpans && counts[1]<=lagi::RbgWindowMaxSpans);
            assert(area[0]+area[1]==352*224);
            for(int y=0;y<224;++y)for(int x=0;x<352;++x)assert(owner[y*352+x]==(oracle(ctrl,x,y)?0:1));
            ++cases;pixels+=owner.size();
        }
    }
    const auto ms=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-start).count();
    std::printf("PASS %u window states, %llu pixel decisions, A/B coverage exact and exclusive (%lld ms host; not Vita timing)\n",
        cases,static_cast<unsigned long long>(pixels),static_cast<long long>(ms));
}

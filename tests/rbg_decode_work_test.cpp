#include "../include/lagi/rbg_decoded_page.h"
#include "baseline.inc"
#include <cassert>
#include <chrono>
#include <cstdio>
#include <vector>
static std::vector<std::uint8_t> raw(0x81000);
template<class Page,class Key> static long long measure(Page& page,Key key,
    std::vector<std::uint32_t>& pixels,unsigned rounds) {
    typename Page::Generations generations{};
    const auto start=std::chrono::steady_clock::now();
    for(unsigned i=0;i<rounds;++i){
        page.reset(); // Force actual cold decode, not a generation-cache hit.
        const auto result=page.update(raw.data(),generations,key,pixels.data(),pixels.size());
        assert(static_cast<unsigned>(result)==1); // Rebuilt
    }
    return std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now()-start).count();
}
int main(){
    for(unsigned i=0;i<raw.size();++i)raw[i]=std::uint8_t((i*37)^(i>>5));
    std::vector<std::uint32_t> current(512*512),prior(512*512);
    lagi::RbgDecodedPage page;baseline::RbgDecodedPage old;
    for(unsigned repeat:{1u,73u,1024u}){
        for(unsigned i=0;i<1024;++i){
            const unsigned name=(i%repeat)|(((i/7)&3)<<10)|(((i/11)&7)<<12);
            raw[0x70000+i*2]=std::uint8_t(name>>8);raw[0x70001+i*2]=std::uint8_t(name);
        }
        for(bool indexed:{false,true})for(bool opaque:{false,true})for(unsigned caos=0;caos<8;++caos){
            const lagi::RbgPageKey key{0x70000,0x8008,std::uint16_t(caos),opaque,indexed};
            const baseline::RbgPageKey oldKey{key.base,key.pncr,key.craofb,opaque,indexed};
            measure(page,key,current,1);measure(old,oldKey,prior,1);
            assert(current==prior); // Exact RGBA/index bytes, including opaque zero.
        }
        const lagi::RbgPageKey key{0x70000,0x8008,0,false,true};
        const baseline::RbgPageKey oldKey{0x70000,0x8008,0,false,true};
        // Alternating order across batches reduces simple warm-up/order bias.
        long long before=0,after=0;
        for(unsigned batch=0;batch<6;++batch){
            if(batch&1){after+=measure(page,key,current,32);before+=measure(old,oldKey,prior,32);}
            else {before+=measure(old,oldKey,prior,32);after+=measure(page,key,current,32);}
        }
        assert(current==prior);
        std::printf("Host cold-page work repeat=%u baseline=%lldus candidate=%lldus (192 rebuilds each; not Vita timing)\n",repeat,before,after);
    }
    std::puts("PASS exact baseline bytes for palette wrap, indexed/RGBA, opaque/transparent zero and repeated/unique flipped tiles");
}

#include "lagi/azel_map_menu.h"
#include <cassert>
#include <climits>
#include <cstdio>

int main(int argc,char** argv){
    LagiAzelMapAvailabilityInputs input;
    auto result=lagiAzelMapAvailabilityFromNative(input);
    assert((result.slots==std::array<std::int32_t,2>{0,-1}) && result.selected==0 && result.worldCover);
    unsigned cases=0;
    for(bool story:{false,true})for(int scene=0;scene<4;++scene)
    for(std::uint32_t route:{0u,1u,0xFFFFFFFFu})
    for(int town:{-1,0,1})for(int target:{-1,0,1})for(bool context:{false,true}){
        input={story,scene,route,static_cast<std::int8_t>(town),target,context};
        result=lagiAzelMapAvailabilityFromNative(input);
        const bool world=story && ((scene==0 && route!=0)||(scene==2 && town!=target));
        assert(result.slots[0]==int(context));
        assert(result.slots[1]==(!story?-1:int(world)));
        assert(result.selected==(context?0:world?1:0));
        assert(result.worldCover==!story);++cases;
    }
    std::vector<std::uint8_t> common(134788);
    const auto put=[&](unsigned p,std::uint32_t v){for(unsigned i=0;i<4;++i)common[p+i]=v>>(24-i*8);};
    std::uint32_t route=99;
    assert(lagiAzelReadFieldWorldMapRoute(nullptr,0,-1,-1,false,false,route) && route==0);
    assert(lagiAzelReadFieldWorldMapRoute(nullptr,0,1,0,true,false,route) && route==0);
    put(0xA3E4,0x80000001);put(0xA4D4+2*4,0x20A3E4);
    assert(lagiAzelReadFieldWorldMapRoute(common.data(),common.size(),1,0,true,true,route) && route==0x80000001);
    assert(lagiAzelReadFieldWorldMapRoute(common.data(),common.size(),2,0,true,false,route) && route==0x80000001);
    assert(lagiAzelReadFieldWorldMapRoute(common.data(),common.size(),0,0,true,true,route) && route==0);
    for(int field:{-1,23,INT_MAX})assert(!lagiAzelReadFieldWorldMapRoute(common.data(),common.size(),field,0,true,true,route) && route==0);
    for(int sub:{-1,INT_MAX})assert(!lagiAzelReadFieldWorldMapRoute(common.data(),common.size(),2,sub,true,true,route) && route==0);
    assert(!lagiAzelReadFieldWorldMapRoute(common.data(),0xA4DC,2,0,true,true,route));
    for(std::uint32_t address:{0x1FFFFFu,0xFFFFFFFFu,0x220E82u,0x20A3E5u,0x220E84u}){
        put(0xA4DC,address);
        assert(!lagiAzelReadFieldWorldMapRoute(common.data(),common.size(),2,0,true,true,route) && route==0);
    }
    if(argc==2){
        FILE* disc=nullptr;assert(!fopen_s(&disc,argv[1],"rb") && disc);
        for(unsigned offset=0;offset<common.size();offset+=2048){
            const auto count=std::min<std::size_t>(2048,common.size()-offset);
            assert(!_fseeki64(disc,(182LL+offset/2048)*2352+16,SEEK_SET));
            assert(fread(common.data()+offset,1,count,disc)==count);
        }
        fclose(disc);
        for(int field:{1,2}){
            for(int sub=0;sub<4;++sub){
                assert(lagiAzelReadFieldWorldMapRoute(common.data(),common.size(),field,sub,true,true,route) && route==1);
            }
            assert(lagiAzelReadFieldWorldMapRoute(common.data(),common.size(),field,4,true,true,route) && route==0);
        }
        for(int field:{0,8,10,12,14,15,17,18,19,20,21,22})
            assert(lagiAzelReadFieldWorldMapRoute(common.data(),common.size(),field,0,true,true,route) && route==0);
        puts("PASS: read-only Disc1 COMMON field1 special route, field2 shared table and null route entries");
    }
    printf("PASS: native availability branches (%u combinations), selection precedence, route gates and malformed-table guards; live gate/parent not connected\n",cases);
}

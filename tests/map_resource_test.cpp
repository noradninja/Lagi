#include "lagi/azel_map_menu.h"
#include <cstdio>
#include <vector>
#include <algorithm>
int main(int argc,char** argv)
{
    if(argc!=2)return 1;
    FILE* disc=nullptr;
    if(fopen_s(&disc,argv[1],"rb") || !disc)return 2;
    std::vector<std::uint8_t> common(134788);
    for(std::size_t offset=0;offset<common.size();offset+=2048){
        const auto count=std::min<std::size_t>(2048,common.size()-offset);
        if(_fseeki64(disc,(182LL+offset/2048)*2352+16,SEEK_SET) ||
            fread(common.data()+offset,1,count,disc)!=count){fclose(disc);return 3;}
    }
    fclose(disc);
    LagiAzelMapDescriptor map;
    if(!lagiAzelReadFieldMapDescriptor(common.data(),common.size(),1,0,13,map) ||
        map.nativeAddress!=0x20E4B4 || map.presentation!=std::array<std::uint8_t,4>{0xFF,0x12,4,0x1F})return 4;
    std::printf("Native field 1/sub 0: %s / %s\n",map.mapFile,map.patternFile);
    if(!lagiAzelReadFieldMapDescriptor(common.data(),common.size(),2,0,13,map) || map.nativeAddress!=0x20E4C0)return 5;
    unsigned available=0;
    for(unsigned field=0;field<23;++field){
        // Each real field has a subfield zero; no speculative secondary index.
        if(lagiAzelReadFieldMapDescriptor(common.data(),common.size(),field,0,1,map)){
            if(!*map.mapFile || !*map.patternFile)return 6;
            ++available;
        }
    }
    if(!available)return 7;
    if(lagiAzelReadFieldMapDescriptor(nullptr,common.size(),1,0,1,map) ||
       lagiAzelReadFieldMapDescriptor(common.data(),common.size(),23,0,1,map) ||
       lagiAzelReadFieldMapDescriptor(common.data(),common.size(),1,1,1,map) ||
       lagiAzelReadFieldMapDescriptor(common.data(),16,1,0,1,map) ||
       lagiAzelReadFieldMapDescriptor(common.data(),common.size(),0,0,1,map))return 8;
    std::printf("PASS: original COMMON field/subfield resource lookup, %u real first-subfield descriptors and invalid/null/truncated rejection (host; not Map rendering)\n",available);
}

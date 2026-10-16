#include <array>
#include <cstdint>
#include <cstdio>

// Read-only MODE1/2352 user-data scan. A literal hit is not proof that the
// containing bytes are instructions; follow up in the owning executable.
int main(int argc,char** argv)
{
    if(argc!=2) {
        std::fprintf(stderr,"Usage: map_gate_literal_scan disc.bin\n");
        return 2;
    }
    FILE* file=nullptr;
    if(fopen_s(&file,argv[1],"rb") || !file) return 3;
    std::array<unsigned char,2352> sector{};
    unsigned lba=0;
    std::size_t count=0;
    while((count=std::fread(sector.data(),1,sector.size(),file))==sector.size()) {
        // Scan every user-data byte, including unaligned script/data values.
        for(unsigned i=16;i+3<16+2048;++i) {
            const auto* p=sector.data()+i;
            const std::uint32_t value=(std::uint32_t(p[0])<<24)|
                (std::uint32_t(p[1])<<16)|(std::uint32_t(p[2])<<8)|p[3];
            const auto physical=value&0x1FFFFFFFu;
            if(physical>=0x24B860 && physical<=0x24B8A0)
                std::printf("lba=%u userOffset=%03X literal=%08X\n",lba,i-16,value);
        }
        ++lba;
    }
    const bool failed=std::ferror(file)!=0 || count!=0;
    std::fclose(file);
    return failed?4:0;
}

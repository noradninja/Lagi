#include "lagi/frontend_font.h"
#include "lagi/disc_image.h"
#include "lagi/platform.h"
#include <cstdio>
#include <vector>

// Read-only initial white palette used by VDP2.cpp::printVdp2String (bank12).
// This is initialized data, not a call into Azel's native text subsystem.
extern unsigned char defaultFontPalettes[16*2*16];

namespace lagi::frontend {
namespace {
LatinFont g_font;
bool g_attempted=false;
bool readLoose(std::vector<std::uint8_t>& bytes)
{
    char path[256];
    std::snprintf(path,sizeof(path),"%s/ASCII.CGZ",platform::filesystem::data_root());
    FILE* input=std::fopen(path,"rb");
    if(!input)return false;
    bool ok=false;
    if(!std::fseek(input,0,SEEK_END)) {
        const long count=std::ftell(input);
        if(count>0 && static_cast<unsigned long>(count)<=LatinFont::maximumCompressedBytes &&
            !std::fseek(input,0,SEEK_SET)) {
            bytes.resize(static_cast<std::size_t>(count));
            ok=std::fread(bytes.data(),1,bytes.size(),input)==bytes.size();
        }
    }
    std::fclose(input);
    if(!ok)bytes.clear();
    return ok;
}
}

bool initializeFont()
{
    if(g_attempted)return g_font.ready();
    g_attempted=true;
    std::vector<std::uint8_t> compressed;
    const auto* palette=defaultFontPalettes+12*32;
    int sourceDisc=0;
    bool loaded=readLoose(compressed) && g_font.load(compressed.data(),compressed.size(),palette);
    for(int disc=1;!loaded && disc<=4;++disc) {
        if(disc::read_file_from_disc(disc,"ASCII.CGZ",compressed,LatinFont::maximumCompressedBytes) &&
            g_font.load(compressed.data(),compressed.size(),palette)) {
            sourceDisc=disc;loaded=true;
        }
    }
    if(loaded)platform::logging::writef(
        "[FrontendFont] source=ASCII.CGZ disc=%d compressed=%u decoded=%u glyphs=95 cell=8x16 advance=8 nativeTextUntouched=1\n",
        sourceDisc,unsigned(compressed.size()),unsigned(g_font.decodedBytes()));
    else platform::logging::writef(
        "[FrontendFont] ASCII.CGZ unavailable/invalid; diagnostic fallback active\n");
    return loaded;
}

const LatinFont& font(){return g_font;}
}

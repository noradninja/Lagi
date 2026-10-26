#include "lagi/lagi_sky_ambient.h"

#include <cassert>

int main() {
    const std::uint8_t blueSky[3]{30,60,90};
    int gray[3]{96,96,96};
    lagi::blend_sky_chroma_preserve_brightness(gray,blueSky);
    assert(gray[0]+gray[1]+gray[2]==288);
    assert(gray[0]<96 && gray[1]==96 && gray[2]>96);

    int colored[3]{120,90,60};
    lagi::blend_sky_chroma_preserve_brightness(colored,blueSky);
    assert(colored[0]+colored[1]+colored[2]==270);
    assert(colored[0]<120 && colored[1]==90 && colored[2]>60);

    const std::uint8_t neutralSky[3]{40,40,40};
    int neutral[3]{120,90,60};
    lagi::blend_sky_chroma_preserve_brightness(neutral,neutralSky);
    assert(neutral[0]==120 && neutral[1]==90 && neutral[2]==60);
}

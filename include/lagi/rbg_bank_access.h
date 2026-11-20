#pragma once
#include <cstdint>
namespace lagi {
// VDP2 manual 6.2: bank role 0=not RBG0,1=coefficient,2=names,3=characters.
// A/B unsplit mode uses the A0/B0 role for both physical half-banks.
inline unsigned rbgBankRole(std::uint16_t ramctl,std::uint32_t address) {
    unsigned bank=(address&0x7ffffu)>>17;
    if(bank<2 && !(ramctl&0x100u))bank=0;
    if(bank>=2 && !(ramctl&0x200u))bank=2;
    return (ramctl>>(bank*2))&3u;
}
inline bool rbgBankReadable(std::uint16_t ramctl,std::uint32_t address,unsigned role) {
    return address<0x80000u && rbgBankRole(ramctl,address)==role;
}
inline void rbgBankPermissions(std::uint16_t ramctl,unsigned role,float output[4]) {
    for(unsigned bank=0;bank<4;++bank)
        output[bank]=rbgBankReadable(ramctl,bank*0x20000u,role)?1.0f:0.0f;
}
}

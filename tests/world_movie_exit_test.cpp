#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
struct GameState {
    std::array<std::uint8_t,32> bitField{};
    void setBit(unsigned bit){bitField[bit/8]|=0x80>>(bit%8);}
    void clearBit(unsigned bit){bitField[bit/8]&=~(0x80>>(bit%8));}
} mainGameState;
static int pending=0,calls=0;
void setNextGameStatus(int status){++calls;if(!pending)pending=status;}
static void exitStatus(int status){switch(status){
#include "production.inc"
default:break;
}}
int main(){
    for(unsigned value=0;value<256;++value){
        for(unsigned i=0;i<32;++i)mainGameState.bitField[i]=std::uint8_t(value+i*7);
        // Start with Garil locked, regardless of the surrounding byte pattern.
        // WORLD entry6's native gate006C is byte13, MSB-first mask08.
        mainGameState.clearBit(0x6C);
        const auto before=mainGameState.bitField;
        pending=calls=0;exitStatus(0x54);
        assert(pending==0x55 && calls==1);
        assert((before[0x6C/8] & (0x80>>(0x6C%8)))==0);
        assert((mainGameState.bitField[0x6C/8] & (0x80>>(0x6C%8)))!=0);
        for(unsigned i=0;i<32;++i){
            unsigned expected=before[i];
            if(i==4)expected|=1;
            if(i==13)expected=(expected&0xDF)|0x18;
            assert(mainGameState.bitField[i]==expected);
        }
    }
    pending=0x4F;calls=0;exitStatus(0x54);assert(pending==0x4F && calls==1);
    for(int status : {0x55,0x56})
        for(unsigned value=0;value<256;++value){
            for(unsigned i=0;i<32;++i)mainGameState.bitField[i]=std::uint8_t(value+i*7);
            const auto before=mainGameState.bitField;
            pending=calls=0;exitStatus(status);
            assert(pending==0x4F&&calls==1&&mainGameState.bitField==before);
            pending=0x58;calls=0;exitStatus(status);
            assert(pending==0x58&&calls==1&&mainGameState.bitField==before);
        }
    const auto before=mainGameState.bitField;
    pending=calls=0;exitStatus(0x53);
    assert(mainGameState.bitField==before && !pending && !calls);
    puts("PASS production status54 story mutations/status55 request, camp55/Garil56 exits to WORLD4F, pending destinations and unrelated bits preserved; device route acceptance pending");
}

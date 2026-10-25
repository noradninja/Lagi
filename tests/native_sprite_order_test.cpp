#include "lagi/native_sprite_order.h"
#include <vector>
#include <cassert>
#include <cstdio>
struct Command {bool hasNativeSortKey;unsigned nativeSortKey,id;};
int main() {
    std::vector<Command> commands;
    for(unsigned i=0;i<65536;++i)commands.push_back({true,i,i});
    lagi::orderNativeSprites(commands);
    for(unsigned i=0;i<32768;++i) {
        assert(commands[i*2].id==32767-i);
        assert(commands[i*2+1].id==65535-i);
    }
    // Panel emitted before the weak markers must still cover them. Equal
    // bucket HUD commands keep native emission order; missing keys are UI.
    commands={{true,0,0},{true,64,1},{true,0x7fff,2},{true,0,3},{false,123,4}};
    lagi::orderNativeSprites(commands);
    const unsigned expected[]={2,1,0,3,4};
    for(unsigned i=0;i<5;++i)assert(commands[i].id==expected[i]);
    puts("PASS native VDP1 stable descending 15-bit packet order: all keys, ties, background/marker/panel and unkeyed UI");
}

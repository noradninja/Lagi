#include <cassert>
#include <cstdio>
using p_workArea=void*;
struct Overlay {int m3=0;} overlay;
struct Manager {Overlay* m10_battleOverlay=&overlay;} manager;
auto* gBattleManager=&manager;
int loads=0,plays=0,found=-1;void* expected;
void loadSoundBanks(int bank,int mode){assert(bank==67&&mode==0);++loads;}
int findSound(int sound){assert(sound==100&&loads>0);return found;}
void playPCM(void* parent,int sound){assert(parent==expected&&sound==100&&overlay.m3==1);++plays;}
#include "lagi_arachnoth_music.inc"
int main(){int parent;expected=&parent;lagiArachnothInitMusic(&parent);assert(loads==1&&plays==1);
 overlay.m3=0;found=0;lagiArachnothInitMusic(&parent);assert(loads==2&&plays==1&&overlay.m3==0);
 found=7;lagiArachnothInitMusic(&parent);assert(loads==3&&plays==1);
 puts("PASS production Arachnoth bank 67 initializer, native playback gate and parent/ownership flag; mocked audio service");}

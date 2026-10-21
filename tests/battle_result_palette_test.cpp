#include <cassert>
#include <cstdint>
#include <string>
#include <cstdio>
using sSaturnPtr=unsigned;
struct {unsigned m14_oldPalette=0,m10_palette=0xA000;} vdp2PrintStatus;
unsigned palette=0,printed=0;
int readSaturnS32(unsigned address){assert(address==0||address==4||address==8);return address==8?palette:address==0?15:6;}
int readSaturnS8(unsigned){assert(false);return 0;}
unsigned readSaturnEA(unsigned address){assert(address==12);return 123;}
std::string readSaturnString(unsigned address){assert(address==123);return "label";}
void vdp2DebugPrintSetPosition(int x,int y){assert(x==15&&y==6);}
void vdp2PrintfLargeFont(const char*,const char*){assert(vdp2PrintStatus.m10_palette==((palette&15)<<12));++printed;}
#include "production.inc"
int main(){for(auto p:{12u,13u,0x1234u}){palette=p;battleResultScreenProgressiveText_printString(0);assert(vdp2PrintStatus.m10_palette==0xA000);}assert(printed==3);puts("PASS generated result text uses native long palette field and restores caller palette; device pending");}

#include <cassert>
#include <cstdio>
using s32=int;using sSaturnPtr=unsigned;
int reads=0,cells=0,resets=0,clears=0;
s32 readSaturnS32(unsigned p){++reads;assert(p==0x060ABAB0||p==0x060ABAB4);return p==0x060ABAB0?18:21;}
struct {int mC_X,m10_Y,m14_Width,m18_Height;}vdp2StringContext;
void setupVDP2StringRendering(int x,int y,int w,int h){assert(x==18&&y==21&&w==16&&h==2);vdp2StringContext={x,y,w,h};}
void clearVdp2TextAreaSub1(int c,int x,int y){assert(c==0&&x>=18&&x<34&&y>=21&&y<23);++cells;}
void vdp2ResetCursor(){++resets;}
void vdp2DebugPrintSetPosition(int x,int y){assert(x==18&&y==21);}
void clearVdp2TextLargeFont(){++clears;}
#include "../src/integration/lagi_battle_result_clear.inc"
int main(){lagiBattleResultClearArea(0x060ABAB0);assert(reads==2&&cells==32&&resets==1&&clears==1);puts("PASS native result clear record reads only coordinates, clears 16x2 region and never dereferences null +C");}

#include <cassert>
#include <cstdio>
#include <vector>
struct Channel{unsigned m0_color;};
struct {Channel m0_fade0{0x8000},m24_fade1{0x8421};unsigned m_48=0xC210,m_4A=0xC631;}g_fadeControls;
struct Field{bool paused=true;Field* getTask(){return this;}void clearPaused(){paused=false;}};
struct sEncouterTask{int m0_status=6,m1=7;Field* m4;};
struct {struct {int m1_isMenuAllowed=0;}m40AC;}graphicEngineStatus;
struct Dragon{int m28_dragon3dModel=0;}dragon,*gDragonState=&dragon;
int encounterTaskVar0=1,clears=0,setups=0;bool ready=false;
struct Fade{Channel* channel;unsigned from,to,frames;};std::vector<Fade>fades;
bool isSoundLoadingFinished(){return ready;}
void clearVdp2TextMemory(){++clears;}
void setupDragonForTown(int*){++setups;}
unsigned convertColorToU32ForFade(unsigned color){return color;}
void fadePalette(Channel* channel,unsigned from,unsigned to,unsigned frames){fades.push_back({channel,from,to,frames});}
#include "resume.inc"
int main(){
    Field field;sEncouterTask task;task.m4=&field;
    resume(&task);assert(field.paused && task.m0_status==6 && fades.empty() && !clears && !setups);
    ready=true;resume(&task);
    assert(!field.paused && task.m0_status==1 && encounterTaskVar0==2 && clears==1 && setups==1);
    assert(graphicEngineStatus.m40AC.m1_isMenuAllowed==7 && fades.size()==2);
    assert(fades[0].channel==&g_fadeControls.m0_fade0 && fades[0].from==0x8000 && fades[0].to==0xC210 && fades[0].frames==30);
    assert(fades[1].channel==&g_fadeControls.m24_fade1 && fades[1].from==0x8421 && fades[1].to==0xC631 && fades[1].frames==30);
    resume(&task);assert(fades.size()==2);
    puts("PASS: actual generated encounter resume waits for sound, restores field/menu ownership and starts both native 30-frame fades to saved field targets exactly once. Mocked services, not hardware fade acceptance.");
}

#include "lagi/azel_map_menu.h"
#include <cassert>
#include <cstdio>
#include <string>
struct s_workArea {bool finished=false;s_workArea* getTask(){return this;}void markFinished(){finished=true;}};
template<class T,class A>struct s_workAreaTemplateWithArg:s_workArea {
 struct TypedTaskDefinition{void(*init)(T*,A);void(*update)(T*);void(*draw)(T*);void(*destroy)(T*);};
};
template<class T,class A>T* createSubTaskWithArg(s_workArea*,A state){auto* t=new T{};T::Init(t,std::move(state));return t;}
struct Common {std::uint32_t m_base;std::uint8_t* m_data;std::size_t m_dataSize;
 unsigned getSaturnPtr(unsigned address){return address;}};Common* gCommonFile;
struct {struct {struct {struct {unsigned m8_newButtonDown=0;}m0_current;}m0_inputDevices[1];}m4514;}graphicEngineStatus;
struct {unsigned m14_numVsyncPerFrame=1;}vblankData;
struct {int m0=77,m4_cursorX=0,m8_cursorY=0,mC_X=0,m10_Y=0;}vdp2StringContext;
std::vector<int> sounds;int boxes=0,clears=0,blueClears=0;
std::string text;std::array<int,4> rect{};
void drawBlueBox(int x,int y,int w,int h,int color){assert(x==8 && y==57 && w==16 && h==4 && color==0x1000);++boxes;}
void clearBlueBox(int x,int y,int w,int h){assert(x==8 && y==57 && w==16 && h==4);++blueClears;}
void setupVDP2StringRendering(int x,int y,int w,int h){rect={x,y,w,h};vdp2StringContext.mC_X=x;vdp2StringContext.m10_Y=y;}
void clearVdp2TextArea(){++clears;}
void drawObjectName(const char* p){text=p;assert(vdp2StringContext.m0==0 && vdp2StringContext.m4_cursorX==10 && vdp2StringContext.m8_cursorY==58);}
void playSystemSoundEffect(int id){sounds.push_back(id);}
namespace lagi::scene_bridge {std::int16_t originX=176,originY=112;
 void capture_local_coordinates(std::int16_t& x,std::int16_t& y){x=originX;y=originY;}}
struct Cursor {int x,y;unsigned sprite,palette;};std::vector<Cursor> cursors;
void drawMultiChoiceVdp1Cursor(int x,int y,unsigned sprite,unsigned palette){cursors.push_back({x,y,sprite,palette});}
#include "map_confirmation_production.inc"
int main(){
 std::vector<std::uint8_t> common(134788);
 common[0x10049]=0x21;common[0x1004B]=0xCC;
 std::memcpy(common.data()+0x100CC,"  YES   NO",11);
 Common object{0x200000,common.data(),common.size()};gCommonFile=&object;
 s_workArea parent;
 auto make=[&](){auto state=std::make_shared<LagiAzelMapConfirmationState>();
  auto* t=static_cast<LagiMapWorldConfirmationTask*>(lagiAzelCreateMapWorldConfirmation(&parent,state));
  assert(state->ready && !state->finished && state->result==0);return t;};
 auto* t=make();auto retained=t->state;assert(!lagiAzelCreateMapWorldConfirmation(&parent,retained));
 t->Update(t);assert(t->phase==2 && text=="  YES   NO" && sounds.back()==3 && boxes==1);
 assert((rect==std::array<int,4>{10,58,12,2}));
 t->Draw(t);assert(cursors.back().x==80 && cursors.back().y==464 && cursors.back().sprite==0x20FFE0 && cursors.back().palette==0x7F0);
 t->Draw(t);assert(t->cursorFrame==1 && t->cursorTicks==0);
 vblankData.m14_numVsyncPerFrame=2;
 for(int i=0;i<7;++i)t->Draw(t);assert(t->cursorFrame==0);
 auto& input=graphicEngineStatus.m4514.m0_inputDevices[0].m0_current.m8_newButtonDown;
 input=0x80;t->Update(t);assert(t->selected==1 && sounds.back()==2);
 auto soundCount=sounds.size();t->Update(t);assert(sounds.size()==soundCount); // no wrap
 input=0xC0;t->Update(t);assert(t->selected==1); // right has priority
 input=0x40;t->Update(t);assert(t->selected==0);
 lagi::scene_bridge::originX=120;lagi::scene_bridge::originY=224;t->Draw(t);
 assert(cursors.back().x==136 && cursors.back().y==352);
 input=7;t->Update(t);assert(retained->result==1 && t->phase==3 && sounds.back()==0); // confirm before B
 t->Draw(t);const auto finalCursorCount=cursors.size();
 t->Update(t);t->Draw(t);assert(t->phase==4 && !retained->finished && cursors.size()==finalCursorCount);
 t->Update(t);t->Draw(t);assert(t->finished && !retained->finished && cursors.size()==finalCursorCount);
 t->Delete(t);delete t;assert(retained->finished && !retained->ready && blueClears==1 && clears==1);
 t=make();t->Update(t);input=0x80;t->Update(t);input=2;t->Update(t);assert(t->state->result==0);t->Delete(t);delete t;
 t=make();t->Update(t);input=1;t->Update(t);assert(t->state->result==0 && sounds.back()==1);t->Delete(t);delete t;
 // Failed initialization must not paint or erase any unrelated menu region.
 const auto priorClears=blueClears;object.m_dataSize=0x1004B;
 auto failed=std::make_shared<LagiAzelMapConfirmationState>();
 t=static_cast<LagiMapWorldConfirmationTask*>(lagiAzelCreateMapWorldConfirmation(&parent,failed));
 assert(t->finished && !failed->ready && failed->running);
 assert(!lagiAzelCreateMapWorldConfirmation(&parent,failed));
 t->Update(t);t->Draw(t);t->Delete(t);delete t;assert(!failed->running);
 assert(failed->finished && blueClears==priorClears);
 object.m_dataSize=common.size();common[0x10048]=0xFF;
 failed=std::make_shared<LagiAzelMapConfirmationState>();
 t=static_cast<LagiMapWorldConfirmationTask*>(lagiAzelCreateMapWorldConfirmation(&parent,failed));
 assert(t->finished && !failed->ready);t->Delete(t);delete t;
 assert(!lagiAzelCreateMapWorldConfirmation(nullptr,failed));
 puts("PASS: production native horizontal confirmation init/input/sound/cursor/cadence/local-origin/cleanup, retained result and malformed COMMON guards; hardware/parent hookup untested");
}

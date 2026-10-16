#include "lagi/azel_map_menu.h"
#include <cassert>
#include <cstdio>
using u32=std::uint32_t;using u16=std::uint16_t;using s32=std::int32_t;
using s16=std::int16_t;
struct s_workArea {bool finished=false;std::vector<s_workArea*> children;
 s_workArea* getTask(){return this;}void markFinished(){finished=true;}};
using p_workArea=s_workArea*;
template<class T,class A>struct s_workAreaTemplateWithArg:s_workArea {
 struct TypedTaskDefinition{void(*init)(T*,A);void(*update)(T*);void(*draw)(T*);void(*destroy)(T*);};
};
template<class T>struct s_workAreaTemplate:s_workArea {
 struct TypedTaskDefinition{void(*init)(T*);void(*update)(T*);void(*draw)(T*);void(*destroy)(T*);};
};
template<class T,class A>T* createSubTaskWithArg(s_workArea* parent,A arg){
 auto* t=new T{};parent->children.push_back(t);
 // Native private cover uses its named Init; selection cursor uses Init too.
 T::Init(t,std::move(arg));return t;
}
template<class T>T* createSubTask(s_workArea* parent){auto* t=new T{};parent->children.push_back(t);T::Init(t);return t;}
struct Layer {int scrollX=0,scrollY=0;};
using s_graphicEngineStatus_40BC=Layer;
struct sMainMenuTaskInitData2 {Layer* m0;u16* m4;};
struct {Layer m40BC_layersConfig[4];} graphicEngineStatus;
struct s_MenuCursorWorkArea {int m0_selectedEntry;Layer* m4;u16* m8;int mC;
 static void menuCursorTaskUpdate(s_MenuCursorWorkArea*);static void menuCursorTaskDraw(s_MenuCursorWorkArea*);};
struct s_menuSprite {int source,size,x,y;};
struct Draw {int source,x,y,palette;};std::vector<Draw> draws;
void drawMenuSprite(s_menuSprite* s,int x,int y,int p){draws.push_back({s->source,x,y,p});}
std::array<std::uint8_t,0x80000> vram{};unsigned writes=0;
u16 getVdp2VramU16(u32 p){assert(p+1<vram.size());return (vram[p]<<8)|vram[p+1];}
void setVdp2VramU16(u32 p,u16 v){assert(p+1<vram.size());vram[p]=v>>8;vram[p+1]=v;++writes;}
struct Common {u32 m_base;std::uint8_t* m_data;std::size_t m_dataSize;};Common* gCommonFile;
#include "native_cursor.inc"
#include "native_highlight.inc"
#include "native_cover.inc"
#include "selection_adapter.inc"
int main(){
 std::vector<std::uint8_t> bytes(0x21000);
 auto put32=[&](unsigned p,u32 v){for(int i=0;i<4;++i)bytes[p+i]=v>>(24-i*8);};
 put32(0xE8A4,0x25E71082);put32(0xE8A8,0x25E71142);
 put32(0xE8B4,0x0604FD6A);put32(0xE8B8,0x20E8AC);
 const std::array<u16,4> positions{25,28,25,76};
 for(int i=0;i<4;++i){bytes[0xE8AC+i*2]=positions[i]>>8;bytes[0xE8AD+i*2]=positions[i];}
 Common common{0x200000,bytes.data(),bytes.size()};gCommonFile=&common;
 LagiAzelMapSelectionData data;assert(lagiAzelReadMapSelectionData(bytes.data(),bytes.size(),data));
 assert(data.layerIndex==1 && data.positions==positions && data.highlightVram[1]==0x71142);
 s_workArea parent;auto state=std::make_shared<LagiAzelMapCursorState>();
 auto* task=static_cast<LagiMapSelectionCursorTask*>(lagiAzelCreateMapSelectionCursor(&parent,state));
 assert(task && state->ready && !lagiAzelCreateMapSelectionCursor(&parent,state));
 bytes[0xE8AD]=99;graphicEngineStatus.m40BC_layersConfig[1]={3,4};
 task->Draw(task);assert(draws.empty());state->selected=0;task->Draw(task);
 assert(draws.size()==1 && draws[0].x==22 && draws[0].y==24 && draws[0].source==0x2030 && draws[0].palette==0x760);
 draws.clear();task->Update(task);task->Draw(task);assert(task->native.mC==40 && draws.size()==2 && draws[0].source==0x2080);
 draws.clear();state->selected=1;task->Draw(task);assert(draws[0].y==72);
 for(int i=0;i<20;++i)task->Update(task);draws.clear();task->Draw(task);assert(draws.size()==1);
 state->selected=2;draws.clear();task->Draw(task);assert(draws.empty());
 auto* cover=static_cast<LagiMapSelectionCoverTask*>(lagiAzelCreateMapWorldUnavailableCover(&parent));
 assert(cover && !cover->finished && cover->children.size()==1);
 auto* nativeCover=static_cast<mainMenuTaskInitSub2TaskWorkArea*>(cover->children[0]);
 assert(nativeCover->spriteIndex==1 && nativeCover->mC==0 && nativeCover->spriteData==cover->data.positions.data());
 bytes[0xE8B3]=111;draws.clear();nativeCover->Draw(nativeCover);
 assert(draws.size()==1 && draws[0].source==0x2118 && draws[0].x==22 && draws[0].y==72 && draws[0].palette==0x630);
 auto* other=static_cast<mainMenuTaskInitSub2TaskWorkArea*>(lagiAzelCreateNativeMenuCover(cover,&cover->config,0,1));
 draws.clear();other->Draw(other);assert(draws[0].source==0x20D8 && draws[0].palette==0x650);
 const auto childCount=cover->children.size();
 assert(!lagiAzelCreateNativeMenuCover(cover,&cover->config,2,0));
 assert(!lagiAzelCreateNativeMenuCover(cover,&cover->config,0,2));
 assert(!lagiAzelCreateNativeMenuCover(nullptr,&cover->config,0,0));
 assert(!lagiAzelCreateNativeMenuCover(cover,nullptr,0,0));
 sMainMenuTaskInitData2 emptyConfig{};
 assert(!lagiAzelCreateNativeMenuCover(cover,&emptyConfig,0,0) && cover->children.size()==childCount);
 assert(!lagiAzelCreateMapWorldUnavailableCover(nullptr));
 // Model task-tree destruction child-first: no child retains stack storage.
 delete nativeCover;delete other;delete cover;
 for(int row=0;row<3;++row)for(int col=0;col<3;++col)setVdp2VramU16(0x71082+row*0x40+col*2,0x6123);
 setVdp2VramU16(0x71082,0x0123);writes=0;
 assert(lagiAzelHighlightMapSelection(0,true) && writes==8 && getVdp2VramU16(0x71084)==0x2123 && getVdp2VramU16(0x71082)==0x123);
 assert(lagiAzelHighlightMapSelection(0,false) && getVdp2VramU16(0x71084)==0x6123);
 writes=0;assert(!lagiAzelHighlightMapSelection(2,true) && writes==0);
 for(u32 bad:{0x0604FD61u,0x0604FD88u,0u}){put32(0xE8B4,bad);assert(!lagiAzelReadMapSelectionData(bytes.data(),bytes.size(),data));}
 put32(0xE8B4,0x0604FD6A);
 for(u32 bad:{0x1FFFFFu,0xFFFFFFFFu,0x220FF9u}){put32(0xE8B8,bad);assert(!lagiAzelReadMapSelectionData(bytes.data(),bytes.size(),data));}
 put32(0xE8B8,0x20E8AC);
 for(u32 bad:{0x25E71083u,0x25E7FF80u,0x25DFFFFEu}){put32(0xE8A4,bad);assert(!lagiAzelHighlightMapSelection(0,true) && writes==0);}
 task->Delete(task);delete task;assert(!state->ready && state->finished);
 auto* badCover=static_cast<LagiMapSelectionCoverTask*>(lagiAzelCreateMapWorldUnavailableCover(&parent));
 assert(badCover->finished && badCover->children.empty());delete badCover;
 auto retry=std::make_shared<LagiAzelMapCursorState>();auto* invalid=static_cast<LagiMapSelectionCursorTask*>(lagiAzelCreateMapSelectionCursor(&parent,retry));
 assert(invalid->finished && !retry->ready);invalid->Delete(invalid);delete invalid;assert(retry->finished);
 assert(!lagiAzelReadMapSelectionData(nullptr,bytes.size(),data));
 assert(!lagiAzelReadMapSelectionData(bytes.data(),0xE8BB,data));
 puts("PASS: production Map selection adapter, generated native cover bridge/Init/Draw/factory, cursor callbacks/highlights, owned coordinates, blink, cleanup and malformed configuration; hardware untested");
}

// Execute the actual navigator service bodies with platform/task mocks.
// This tests address rebasing and ownership, not Vita input/rendering.
#include <cassert>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <cstdio>
using u8=std::uint8_t; using u32=std::uint32_t; using s32=std::int32_t; using s16=std::int16_t;
struct s_heapNode{u32 m_magic; s_heapNode* m_nextNode;u32 m_size;void* getUserData(){return this+1;}};
struct s_task{s_heapNode* m_heapNode=nullptr;};
struct s_workArea{s_task task;s_task* getTask(){return &task;}};
#include "heap_body.inc"
struct sSaturnMemoryFile;
struct sSaturnPtr { u32 m_offset; sSaturnMemoryFile* m_file; };
struct sSaturnMemoryFile {
 std::string m_name; u8* m_data=nullptr; u32 m_dataSize=0, m_base=0;
 sSaturnPtr getSaturnPtr(u32 address) {return {address,this};}
};
sSaturnMemoryFile overlay;
sSaturnMemoryFile* gFLD_A3=&overlay;
void* getSaturnPtr(sSaturnPtr p) {
 assert(p.m_offset>=p.m_file->m_base && p.m_offset-p.m_file->m_base<p.m_file->m_dataSize);
 return p.m_file->m_data+p.m_offset-p.m_file->m_base;
}
u32 readSaturnU32(sSaturnPtr p) {
 auto* b=static_cast<u8*>(getSaturnPtr(p));
 assert(p.m_offset-p.m_file->m_base+4<=p.m_file->m_dataSize);
 return (u32(b[0])<<24)|(u32(b[1])<<16)|(u32(b[2])<<8)|b[3];
}
struct s_multiChoice {s16* m0_choiceTable; s32 m4_currentChoice, m8_numChoices, mC;};
struct s_multiChoiceTask {sSaturnPtr strings; s32 count; s32* result;};
struct s_fieldScriptWorkArea : s_workArea {s_multiChoice* m44_multiChoiceData=nullptr; s_multiChoiceTask* m3C_multichoiceTask=nullptr;};
struct s_FieldRadar : s_workArea {s32 m14_encounterCount=0,m18_currentSelection=0,m24_filteredChoiceCount=0; void* m1C_encounterList=nullptr; s32* m20_choiceArray=nullptr;};
struct Subfield {s_FieldRadar* m33C_fieldRadar; s_fieldScriptWorkArea* m34C_ptrToE;};
struct Field {Subfield* m8_pSubFieldData;};
s_FieldRadar radar; s_fieldScriptWorkArea script; Subfield subfield{&radar,&script}; Field field{&subfield};
Field* getFieldTaskPtr(){return &field;}
struct {u8 bitField[512]{};} mainGameState;
u8 bitMasks[]={0x80,0x40,0x20,0x10,8,4,2,1};
std::string selectedText;
void setGlobalStringTableEntry(int index,const char* text){assert(index==2);selectedText=text;}
unsigned freedStyles=0;
void dispatchTutorialMultiChoiceSub1(){
 assert(!script.m3C_multichoiceTask);
 freeHeapForTask(&script,script.m44_multiChoiceData->m0_choiceTable);
 freeHeapForTask(&script,script.m44_multiChoiceData);
 script.m44_multiChoiceData=nullptr;
 ++freedStyles;
}
s_multiChoiceTask* updateMultiChoice(s_fieldScriptWorkArea* parent,s_multiChoiceTask** out,s32* result,s32 count,sSaturnPtr table,s16* colors,s32 selected){
 assert(parent==&script && count>0 && selected>=0 && selected<count && colors);
 // Consume table exactly like native readSaturnEA: same file, rebased EA.
 for(s32 i=0;i<count;++i){
  sSaturnPtr slot{table.m_offset+u32(i*4),table.m_file};
  const u32 ea=readSaturnU32(slot);
  assert(ea>=u32(count*4) && ea<table.m_file->m_dataSize);
  auto* text=static_cast<char*>(getSaturnPtr({ea,table.m_file}));
  assert(std::strlen(text)<table.m_file->m_dataSize-ea);
 }
 auto* dialog=new s_multiChoiceTask{table,count,result};
 *out=dialog;
 return dialog;
}

#include "navigator_body.inc"

void write32(u32 ea,u32 v){auto* p=static_cast<u8*>(getSaturnPtr(overlay.getSaturnPtr(ea)));for(unsigned i=0;i<4;++i)p[i]=u8(v>>(24-i*8));}
constexpr u32 textA=0x06081000,textB=0x06081040,textBack=0x0608122b;
void prepare(bool withBack){
 assert(!script.m3C_multichoiceTask && !script.m44_multiChoiceData && !radar.m20_choiceArray && views.empty());
 const s32 count=withBack?3:2;
 radar.m24_filteredChoiceCount=count;
 radar.m20_choiceArray=static_cast<s32*>(allocateHeapForTask(&radar,count*sizeof(s32)));
 radar.m20_choiceArray[0]=textA;radar.m20_choiceArray[1]=textB;
 if(withBack)radar.m20_choiceArray[2]=textBack;
 assert(!script.getTask()->m_heapNode);
 auto* choice=static_cast<s_multiChoice*>(allocateHeapForTask(&script,sizeof(s_multiChoice)));
 choice->m0_choiceTable=static_cast<s16*>(allocateHeapForTask(&script,count*sizeof(s16)));
 choice->m4_currentChoice=withBack?1:0;choice->m8_numChoices=count;
 script.m44_multiChoiceData=choice;
}
void close(int result){
 auto* task=script.m3C_multichoiceTask;
 assert(task && views.size()==1);
 *task->result=result;
 lagiReleaseFieldNavigatorDialog(task);
 script.m3C_multichoiceTask=nullptr;
 delete task;
 assert(views.empty());
}
int main(){
 // Execute actual task-heap unlinking for middle, head and tail nodes.
 s_workArea heapOwner;
 auto* a=allocateHeapForTask(&heapOwner,4);auto* b=allocateHeapForTask(&heapOwner,8);auto* c=allocateHeapForTask(&heapOwner,12);
 freeHeapForTask(&heapOwner,b);assert(heapOwner.task.m_heapNode->getUserData()==a && heapOwner.task.m_heapNode->m_nextNode->getUserData()==c);
 freeHeapForTask(&heapOwner,a);assert(heapOwner.task.m_heapNode->getUserData()==c);
 freeHeapForTask(&heapOwner,c);assert(!heapOwner.task.m_heapNode);
 freeHeapForTask(&heapOwner,nullptr);
 std::vector<u8> data(0x42000);overlay.m_base=0x06054000;overlay.m_data=data.data();overlay.m_dataSize=u32(data.size());
 std::strcpy(static_cast<char*>(getSaturnPtr(overlay.getSaturnPtr(textA))),"Target A");
 std::strcpy(static_cast<char*>(getSaturnPtr(overlay.getSaturnPtr(textB))),"Target B");
 std::strcpy(static_cast<char*>(getSaturnPtr(overlay.getSaturnPtr(textBack))),"Deactivate Navigator");
 write32(0x06095a28,0x06080fa5);write32(0x06080fa8,textBack);
 assert(lagiFieldNavigatorDefaultTextEA()==textBack);
 u32 entries[15]={textBack,0,0,0,0,textA,32,0,0,0,textB,33,0,0,0};
 radar.m1C_encounterList=allocateHeapForTask(&radar,sizeof(entries));std::memcpy(radar.m1C_encounterList,entries,sizeof(entries));
 radar.m14_encounterCount=3;mainGameState.bitField[4]=0xc0;
 prepare(false);assert(lagiOpenFieldNavigatorDialog()==0 && views.size()==1);
 assert(lagiFinishFieldNavigatorDialog()==0); // Task still owns strings/styles.
 auto* task=script.m3C_multichoiceTask;
 assert(std::strcmp(static_cast<char*>(getSaturnPtr({readSaturnU32(task->strings),task->strings.m_file})),"Target A")==0);
 assert(lagiOpenFieldNavigatorDialog()==0 && views.size()==1); // No duplicate open.
 close(1);assert(lagiFinishFieldNavigatorDialog()==1 && radar.m18_currentSelection==2 && selectedText=="Target B");
 for(unsigned i=0;i<64;++i){prepare(true);lagiOpenFieldNavigatorDialog();close(-1);assert(lagiFinishFieldNavigatorDialog()==1 && radar.m18_currentSelection==2 && selectedText=="Target B");}
 prepare(true);lagiOpenFieldNavigatorDialog();close(2);assert(lagiFinishFieldNavigatorDialog()==1 && radar.m18_currentSelection==0 && selectedText.empty());
 assert(freedStyles==66 && views.empty() && !script.getTask()->m_heapNode);
 assert(radar.getTask()->m_heapNode->getUserData()==radar.m1C_encounterList && !radar.getTask()->m_heapNode->m_nextNode);
 // Forced native parent teardown releases the same backing view; no finish
 // routine is required to release strings. Native heap cleanup is mocked.
 prepare(false);lagiOpenFieldNavigatorDialog();close(-1);
 dispatchTutorialMultiChoiceSub1();freeHeapForTask(&radar,radar.m20_choiceArray);radar.m20_choiceArray=nullptr;
 freeHeapForTask(&radar,radar.m1C_encounterList);radar.m1C_encounterList=nullptr;
 assert(!radar.getTask()->m_heapNode && !script.getTask()->m_heapNode);
 assert(views.empty());
 std::puts("PASS: actual navigator and task-heap open/wait/select/cancel/reopen/deactivate/teardown bodies, unlinking and rebased strings; not Vita execution");
}

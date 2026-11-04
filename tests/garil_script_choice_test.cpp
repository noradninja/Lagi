#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
using u32=std::uint32_t;using s32=std::int32_t;using s8=std::int8_t;
struct File {} a5,a3;File* gFLD_A5=&a5;
struct sSaturnPtr {u32 m_offset;File* m_file;sSaturnPtr operator+(int n)const{return {m_offset+u32(n),m_file};}sSaturnPtr operator-(int n)const{return *this+-n;}};
std::array<unsigned char,256> bytes{};
s8 readSaturnS8(sSaturnPtr p){assert(p.m_file==&a5);return s8(bytes.at(p.m_offset));}
struct Dialog {int m0_Status=0;} dialog;
struct s_fieldScriptWorkArea {Dialog* m3C_multichoiceTask=nullptr;s32 m54_currentResult=42;};
int starts=0,lastCount=0;u32 lastChoices=0;bool failCreation=false;
void startDialogTask(s_fieldScriptWorkArea* owner,Dialog** child,s32* result,int count,sSaturnPtr choices){
 assert(child==&owner->m3C_multichoiceTask&&result==&owner->m54_currentResult&&choices.m_file==&a5);
 ++starts;lastCount=count;lastChoices=choices.m_offset;*child=failCreation?nullptr:&dialog;
}
#include "lagi/garil_script_choice.h"
#include "choice.inc"
int main(){
 s_fieldScriptWorkArea state;sSaturnPtr pc{64,&a3};
 assert(lagiAzelRunGarilChoice30(&state,pc)==LagiGarilChoiceStep::Unsupported&&pc.m_offset==64&&!starts);
 pc.m_file=&a5;gFLD_A5=nullptr;
 assert(lagiAzelRunGarilChoice30(&state,pc)==LagiGarilChoiceStep::Unsupported&&!starts);gFLD_A5=&a5;
 for(int alignment=0;alignment<4;++alignment)
 for(int count:{-128,-3,-1,0,1,3,127}){
  const unsigned operand=64+alignment;bytes[operand]=static_cast<unsigned char>(count);
  state.m3C_multichoiceTask=nullptr;pc={operand,&a5};const int before=starts;
  auto step=lagiAzelRunGarilChoice30(&state,pc);
  if(!count){assert(step==LagiGarilChoiceStep::Continue&&pc.m_offset==operand+1&&starts==before);}
  else {
   assert(step==LagiGarilChoiceStep::Yield&&pc.m_offset==operand-1&&starts==before+1);
   assert(lastCount==count&&lastChoices==((operand+4)&~3u)); // do not negate count
  }
  for(int phase:{0,1,2,3,5}){
   state.m3C_multichoiceTask=&dialog;dialog.m0_Status=phase;pc={operand,&a5};const int previous=starts;
   assert(lagiAzelRunGarilChoice30(&state,pc)==LagiGarilChoiceStep::Yield);
   assert(pc.m_offset==operand-1&&starts==previous&&state.m54_currentResult==42);
  }
  dialog.m0_Status=4;pc={operand,&a5};const int previous=starts;
  assert(lagiAzelRunGarilChoice30(&state,pc)==LagiGarilChoiceStep::Yield);
  const unsigned expected=count>0?((operand+4)&~3u)+unsigned(count)*4:operand+1;
  assert(pc.m_offset==expected&&pc.m_file==&a5&&starts==previous&&state.m54_currentResult==42);
 }
 failCreation=true;state.m3C_multichoiceTask=nullptr;pc={64,&a5};bytes[64]=3;
 assert(lagiAzelRunGarilChoice30(&state,pc)==LagiGarilChoiceStep::Yield&&pc.m_offset==63&&!state.m3C_multichoiceTask);
 puts("PASS A5 choice30 sign, alignment, no-choice continuation, dialog creation/wait/completion, result retention and owner isolation; native dialog mocked");
}

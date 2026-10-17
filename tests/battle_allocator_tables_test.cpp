#include "lagi/native_allocator_tables.h"
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <unordered_map>
#include <utility>
using u8=std::uint8_t;using u32=std::uint32_t;
struct s_workArea{};using p_workArea=s_workArea*;
struct npcFileDeleter{};
struct s_fileEntry{std::string mFileName;int m0_fileID,m4_fileSize,m8_refcount;npcFileDeleter* mC_fileBundle;};
struct s_dramAllocationNode{s_dramAllocationNode* m_pNext;u32 size;};
struct s_dramAllocator{s_dramAllocationNode* m0_buffer;u32 m4_pNext;u8* m8_allocationStart;u8* mC_allocationEnd;s_dramAllocator* m10_nextNode;u32 m14;};
s_dramAllocator* dramAllocatorHead=nullptr;std::vector<s_fileEntry> dramAllocatorEnd;
void* vdp1AllocatorHead=nullptr;
namespace lagi::platform::logging{template<class... T>void writef(const char*,T...){assert(false && "Missing allocator table owner");}}
std::vector<void*> heaps;unsigned freeCalls=0;
void* allocateHeapForTask(s_workArea*,size_t size){auto* p=std::calloc(1,size);heaps.push_back(p);return p;}
void loadRamResource(s_workArea*){}
void freeRamResource(){++freeCalls;}
int findMandatoryFileOnDisc(const char*){return 1;}
u32 getFileSizeFromFileId(const char*){return 64;}
void addToMemoryLayout(u8*,u32){}
#include "service.inc"
#include "push.inc"
#include "pop.inc"
#include "reset.inc"
int main(){
    s_workArea task;alignas(s_dramAllocationNode) u8 field[256]{},battle[256]{},nested[256]{};
    const char* fieldAssets[]={"FIELD.MCB","FIELD.CGB",nullptr};
    const char* battleAssets[]={"BATTLE.MCB",reinterpret_cast<const char*>(-1),"BATTLE.CGB",nullptr};
    npcFileDeleter fieldNpc,battleNpc;
    initDramAllocator(&task,field,sizeof(field),fieldAssets);
    const auto fieldOwner=dramAllocatorHead;assert(dramAllocatorEnd.size()==2);
    const auto* fieldTable=dramAllocatorEnd.data();
    dramAllocatorEnd[0].m8_refcount=7;dramAllocatorEnd[0].mC_fileBundle=&fieldNpc;
    initDramAllocator(&task,battle,sizeof(battle),battleAssets);
    const auto battleOwner=dramAllocatorHead;assert(battleOwner->m10_nextNode==fieldOwner);
    assert(dramAllocatorEnd.size()==3 && dramAllocatorEnd[1].m0_fileID==-1);
    dramAllocatorEnd[2].m8_refcount=4;dramAllocatorEnd[2].mC_fileBundle=&battleNpc;
    initDramAllocator(&task,nested,sizeof(nested),nullptr);assert(dramAllocatorEnd.empty());
    freeRamResources(&task);
    assert(dramAllocatorHead==battleOwner && dramAllocatorEnd.size()==3);
    assert(dramAllocatorEnd[2].m8_refcount==4 && dramAllocatorEnd[2].mC_fileBundle==&battleNpc);
    freeRamResources(&task);
    assert(dramAllocatorHead==fieldOwner && dramAllocatorEnd.size()==2);
    assert(dramAllocatorEnd.data()==fieldTable);
    assert(dramAllocatorEnd[0].mFileName=="FIELD.MCB" && dramAllocatorEnd[0].m8_refcount==7 && dramAllocatorEnd[0].mC_fileBundle==&fieldNpc);
    freeRamResources(&task);assert(!dramAllocatorHead && dramAllocatorEnd.empty() && freeCalls==3);
    freeRamResources(&task);assert(freeCalls==3);
    initDramAllocator(&task,field,sizeof(field),fieldAssets);
    initDramAllocator(&task,battle,sizeof(battle),battleAssets);
    resetTempAllocators();assert(!dramAllocatorHead && dramAllocatorEnd.empty() && parentTables.empty());
    for(auto* allocation:heaps)std::free(allocation);
    puts("PASS: generated native allocator push/pop/reset plus production table service restore parent filenames/refcounts/bundle identities through nested empty/nonempty scopes; mocked heap/resource I/O");
}

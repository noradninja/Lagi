#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>
using u8=uint8_t;using u16=uint16_t;using u32=uint32_t;
struct s_vdp1AllocationNode {s_vdp1AllocationNode* m_0=nullptr;u16 m4_vdp1Memory=0,m6_size=0;};
struct s_vdp1Allocator {s_vdp1Allocator* m0_nextAllocator=nullptr;s_vdp1AllocationNode* m4_nextNode=nullptr;
 s_vdp1AllocationNode m8_freeHead,m10_allocatedHead,m18_list[0x40];};
s_vdp1Allocator* vdp1AllocatorHead=nullptr;
u8 vram[0x80000];int invalidations=0;
u8* getVdp1Pointer(u32 address){assert(address>=0x25C00000&&address<0x25C80000);return vram+address-0x25C00000;}
void invalidateVdp1TextureRange(u32 offset,u32 size){assert(offset+size<=sizeof(vram));++invalidations;}
#include "free.inc"
struct npcFileDeleter {int mC=-1;int cleanup=0;s_vdp1AllocationNode* allocation=nullptr;static void Delete(npcFileDeleter*);};
struct Entry {int m8_refcount=0;npcFileDeleter* mC_fileBundle=nullptr;};
struct CheckedTable:std::vector<Entry>{Entry& operator[](size_t n){assert(n<size());return std::vector<Entry>::operator[](n);}} dramAllocatorEnd;
void npcFileDeleterCleanup(npcFileDeleter* p){++p->cleanup;if(p->allocation){vdp1Free(reinterpret_cast<u8*>(p->allocation));p->allocation=nullptr;}}
#include "delete.inc"
int main(){
 s_vdp1Allocator oldPool,newPool;vdp1AllocatorHead=&newPool;
 auto& retired=oldPool.m18_list[8];retired.m4_vdp1Memory=0x100;retired.m6_size=8;
 npcFileDeleter old;old.mC=8;old.allocation=&retired;
 dramAllocatorEnd.resize(4);std::memset(vram,0x7b,sizeof(vram));
 npcFileDeleter::Delete(&old);assert(old.cleanup==1&&invalidations==0&&vram[0x800]==0x7b);
 dramAllocatorEnd.clear();npcFileDeleter::Delete(&old);assert(old.cleanup==2);
 // A reused in-range slot belongs to the new task, never the old deleter.
 npcFileDeleter replacement;dramAllocatorEnd.resize(12);dramAllocatorEnd[8]={3,&replacement};
 npcFileDeleter::Delete(&old);assert(dramAllocatorEnd[8].mC_fileBundle==&replacement&&dramAllocatorEnd[8].m8_refcount==3);
 // Active ownership still detaches and frees normally.
 npcFileDeleter live;live.mC=2;auto& node=newPool.m18_list[2];node.m4_vdp1Memory=0x100;node.m6_size=8;live.allocation=&node;
 dramAllocatorEnd[2]={1,&live};npcFileDeleter::Delete(&live);
 assert(live.cleanup==1&&invalidations==1&&vram[0x800]==0&&dramAllocatorEnd[2].mC_fileBundle==nullptr);
 // A still-active nested allocator must receive its own node, not head's.
 newPool.m0_nextAllocator=&oldPool;oldPool.m8_freeHead.m_0=nullptr;
 vdp1Free(reinterpret_cast<u8*>(&retired));assert(oldPool.m8_freeHead.m_0==&retired&&newPool.m8_freeHead.m_0==&node&&invalidations==2);
 vdp1Free(reinterpret_cast<u8*>(uintptr_t(1)));assert(invalidations==2);
 vdp1AllocatorHead=nullptr;vdp1Free(reinterpret_cast<u8*>(&node));assert(invalidations==2);
 old.mC=-1;npcFileDeleter::Delete(&old);
 puts("PASS generated resource Delete/VDP1 free: shorter/empty/reused tables, active/nested ownership, retired nodes and no stale VRAM clears; payload cleanup mocked");
}

#include "lagi/lagi_azel_upstream_prelude.h"
#include "lagi/native_allocator_tables.h"
#include "lagi/platform.h"
#include "mainMenuDebugTasks.h"
#include <unordered_map>
#include <utility>

namespace {
// Saturn stores each file table inline at allocator +18. The port's vector
// cannot be byte-copied there; retain each table under its native owner.
std::unordered_map<s_dramAllocator*,std::vector<s_fileEntry>> parentTables;
}
void lagiPushDramFileTable(s_dramAllocator* allocator)
{
    if(allocator && allocator->m10_nextNode)
        parentTables[allocator->m10_nextNode]=std::move(dramAllocatorEnd);
}
void lagiPopDramFileTable(s_dramAllocator* allocator)
{
    parentTables.erase(allocator);
    const auto parent=dramAllocatorHead;
    const auto saved=parentTables.find(parent);
    if(parent && saved!=parentTables.end()){
        dramAllocatorEnd=std::move(saved->second);
        parentTables.erase(saved);
    }else{
        dramAllocatorEnd.clear();
        if(parent)lagi::platform::logging::writef("[BattleAllocator] missing parent file table owner=%p\n",static_cast<void*>(parent));
    }
}
void lagiResetDramFileTables()
{
    parentTables.clear();
}

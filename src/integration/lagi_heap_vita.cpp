#include "lagi/lagi_compat.h"
#include "heap.h"
#include "task.h"
#include "VDP1.h"
#include "VDP2.h"
#include "common.h"

static const u32 HEAP_MAGIC = 0xCAFEC0DE;

void initHeap() {}

void* allocateHeap(u32 size)
{
    return size ? new u8[size] : nullptr;
}

void* allocateHeapForTask(s_workArea* workArea, u32 size)
{
    if (!size) return nullptr;
    s_heapNode** node = &workArea->getTask()->m_heapNode;
    while (*node) node = &(*node)->m_nextNode;

    const u32 allocatedSize = size + sizeof(s_heapNode);
    u8* raw = new u8[allocatedSize];
    std::memset(raw, 0, allocatedSize);

    s_heapNode* newNode = reinterpret_cast<s_heapNode*>(raw);
    newNode->m_magic = HEAP_MAGIC;
    newNode->m_nextNode = nullptr;
    newNode->m_size = size;
    *node = newNode;
    return newNode->getUserData();
}

void freeHeap(void* data) { delete[] reinterpret_cast<u8*>(data); }

void freeHeapForTask(s_workArea* workArea, void* data)
{
    if (!data) return;
    // Original 060151FE searches the owning list and unlinks at 06015234.
    // Deleting only the storage leaves allocateHeapForTask walking freed nodes
    // when a native menu is opened again.
    assert(workArea && workArea->getTask());
    auto** link = &workArea->getTask()->m_heapNode;
    while (*link && (*link)->getUserData() != data) link = &(*link)->m_nextNode;
    assert(*link); // Native 06015220..24 rejects a non-owned allocation.
    if (!*link) return;
    s_heapNode* node = *link;
    assert(node->m_magic == HEAP_MAGIC);
    *link = node->m_nextNode;
    node->m_magic = 0;
    node->m_nextNode = nullptr;
    delete[] reinterpret_cast<u8*>(node);
}

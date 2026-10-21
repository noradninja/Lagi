#pragma once
struct s_dramAllocator;
void lagiPushDramFileTable(s_dramAllocator* allocator);
void lagiPopDramFileTable(s_dramAllocator* allocator);
void lagiResetDramFileTables();

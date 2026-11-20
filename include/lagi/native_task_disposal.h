#pragma once
namespace lagi::tasks {
// Host-only ownership: Azel work areas have virtual callbacks but no virtual
// destructor. Opt in only when the creator knows the concrete C++ type.
using WorkAreaDisposer=void(*)(void*);
void register_work_area_disposer(void* area,WorkAreaDisposer disposer);
}

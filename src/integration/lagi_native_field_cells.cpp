#include "lagi/lagi_azel_upstream_prelude.h"
#include "lagi/native_field_cells.h"

namespace {
lagi::NativeFieldCellFactories<s_visdibilityCellTask, s_DataTable2Sub0> factories;
}
void lagiRegisterNativeFieldCellFactory(const sSaturnMemoryFile* owner, unsigned address,
                                      LagiNativeFieldCellFactory factory) {
    factories.bind(owner, address, factory);
}
bool lagiDispatchNativeFieldCellFactory(s_visdibilityCellTask* cell,
                                       s_DataTable2Sub0& placement, int memoryArea) {
    return factories.dispatch(placement.m0_function.m_file, placement.m0_function.m_offset,
                              cell, placement, memoryArea);
}

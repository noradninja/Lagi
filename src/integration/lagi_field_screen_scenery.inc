// FLD_A5 060780A2..06078144: third cell list, 16-byte records.
// One active field layout owns this decoded layer; loading a new layout replaces it.
static const s_DataTable3* lagiScreenSceneryLayout = nullptr;
static std::vector<std::vector<s_grid2>> lagiScreenSceneryCells;

static void lagiDrawFieldScreenScenery(s_visdibilityCellTask* cell, s32 farClip)
{
    auto* grid = getFieldTaskPtr()->m8_pSubFieldData->m348_pFieldCameraTask1;
    if (grid->m30 != lagiScreenSceneryLayout || cell->m14_index < 0 ||
        unsigned(cell->m14_index) >= lagiScreenSceneryCells.size()) return;
    // The root is the field view. Keep these camera-facing models dynamic.
    lagi::azel_bridge::begin_view_relative_submission_scope();
    for (const auto& entry : lagiScreenSceneryCells[cell->m14_index]) {
        const u16 modelOffset = readSaturnU16(entry.m0);
        auto* model = cell->m0_memoryLayout.m0_mainMemoryBundle->get3DModel(modelOffset);
        ++grid->m12E0;
        grid->m12F0 += model->m4_numVertices;
        if (grid->m12FC_isObjectClipped(&entry.m4, farClip)) continue;
        ++grid->m12E2;
        pushCurrentMatrix();
        translateCurrentMatrix(&entry.m4);
        if (gridCellDraw_GetDepthRange(pCurrentMatrix->m[2][3]) <= grid->m1300) {
            // Common 0601E424: preserve view translation, replace basis by
            // diag(-1,1,1). This differs from the second list's rolled billboard.
            for (unsigned row=0; row<3; ++row)
                for (unsigned col=0; col<3; ++col)
                    pCurrentMatrix->m[row][col] = row==col ? (row==0 ? -0x10000 : 0x10000) : 0;
            addObjectToDrawList(model);
            const u16 collision = readSaturnU16(entry.m0 + 2);
            if (collision) {
                // Native 06077010 leaves translation intact and resets this basis.
                callGridCellDraw_normalSub2(cell->m0_memoryLayout.m0_mainMemoryBundle,
                    collision);
            }
        }
        popMatrix();
    }
    lagi::azel_bridge::end_view_relative_submission_scope();
}

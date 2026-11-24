// Common Saturn 0601489E and 06014A1C: stream a five-cell column
// through the same 8x8 ring used by battleGrid_createCell/deleteCell.
void battleGrid_cellFunc1(s32 index, sWorldGrid* grid)
{
    const s32 worldX = grid->m10_currentX + index;
    const s32 maskX = grid->m0_sizeX - 1;
    const s32 maskY = grid->m4_sizeY - 1;
    const s32 wrappedX = worldX & maskX;
    const s32 offsetX = s32(u32(worldX & ~maskX) * u32(grid->m28_cellSize));
    static const sWorldGridCellTask::TypedTaskDefinition definition = {
        sBattleEnvironmentGridCell_Init, nullptr, sBattleEnvironmentGridCell_Draw, nullptr};
    for (s32 row=-2; row<=2; ++row) {
        const s32 worldY = grid->m14_currentY + row;
        const s32 wrappedY = worldY & maskY;
        const s32 offsetY = s32(u32(worldY & ~maskY) * u32(grid->m28_cellSize));
        const auto data = grid->m38_EnvironmentSetup->cells[wrappedX][wrappedY];
        auto* cell = createSubTaskWithArgWithCopy<sWorldGridCellTask>(
            grid->m34_dataBuffer, data, &definition);
        grid->m40_cellTasks[(grid->mC + row)&7][(grid->m8 + index)&7] = cell;
        if (!cell) continue;
        cell->mC_position[0] += offsetX;
        cell->mC_position[2] += offsetY;
        battleGrid_createCellSub0(cell, wrappedX, wrappedY, offsetX, offsetY);
    }
}

void battleGrid_cellFunc2(s32 index, sWorldGrid* grid)
{
    for (s32 row=-2; row<=2; ++row) {
        auto*& cell = grid->m40_cellTasks[(grid->mC + row)&7][(grid->m8 + index)&7];
        if (!cell) continue;
        cell->getTask()->markFinished();
        cell = nullptr;
    }
}

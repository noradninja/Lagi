// BTL_A5 0605A0C4..0605A2C2: all versions use the same VDP2 setup and draw.
template <unsigned GridAddress, int GridWidth, int GridHeight>
static void lagiInitGarilBattleEnvironment(sVdp2PlaneTask* task)
{
    BTL_A5_env_InitVdp2(task);
    auto* bundle = dramAllocatorEnd[6].mC_fileBundle;
    if constexpr (GridAddress != 0) {
        gBattleManager->m10_battleOverlay->m8_gridTask->m1C8_flags |= 0x10;
        auto grid = readGrid(g_BTL_A5->getSaturnPtr(GridAddress), GridWidth, GridHeight);
        initGridForBattle(bundle, grid, GridWidth, GridHeight, 0x200000);
    }
    task->m58 = bundle;
}

template <unsigned GridAddress, int GridWidth = 2, int GridHeight = 2>
static void lagiSpawnGarilBattleEnvironment(p_workArea parent)
{
    static const sVdp2PlaneTask::TypedTaskDefinition definition = {
        &lagiInitGarilBattleEnvironment<GridAddress, GridWidth, GridHeight>,
        nullptr, &BTL_A5_env_Draw, nullptr,
    };
    createSubTask<sVdp2PlaneTask>(parent, &definition);
}

void lagiCreateGarilBattleEnvironment(p_workArea parent, unsigned address)
{
    switch (address) {
    case 0x0605A724: lagiSpawnGarilBattleEnvironment<0x060AE8EC>(parent); break;
    case 0x0605A72E: lagiSpawnGarilBattleEnvironment<0x060AEDA0>(parent); break;
    case 0x0605A742: lagiSpawnGarilBattleEnvironment<0>(parent); break;
    case 0x0605A74C: lagiSpawnGarilBattleEnvironment<0x060AEA28>(parent); break;
    case 0x0605A756: lagiSpawnGarilBattleEnvironment<0x060AEB64>(parent); break;
    case 0x0605A760: lagiSpawnGarilBattleEnvironment<0x060AF1EC, 4, 4>(parent); break;
    }
}

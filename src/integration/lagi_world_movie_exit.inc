    case 0x14: // EVT008
        // MAIN 06027DEA sets the Camp gift prerequisite and queues battle 15.
        mainGameState.setBit(5 * 8 + 6);
        setNextGameStatus(0x15);
        if (mainGameState.gameStats.m1_dragonLevel != DR_LEVEL_2_STRIPE_WING)
        {
            updateDragonIfCursorChanged(DR_LEVEL_2_STRIPE_WING);
        }
        return 0;

    case 0x54: // WORLD's Village of Cainus arrival movie
        // MAIN 06027AA4's slot54 resolves to06027C7A. The native handler
        // updates these MSB-first story bits, then requests55/TWN_CAMP.
        mainGameState.setBit(4 * 8 + 7);
        mainGameState.clearBit(13 * 8 + 2);
        mainGameState.setBit(13 * 8 + 4);
        mainGameState.setBit(13 * 8 + 3);
        setNextGameStatus(0x55);
        break;
    case 0x55: // Cainus camp
    case 0x56: // Garil Desert
        // Both native jump-table slots resolve to MAIN060283F0: request
        // WORLD4F without story mutations. setNextGameStatus retains any
        // already pending authored destination, just like MAIN06027110.
        setNextGameStatus(0x4F);
        break;

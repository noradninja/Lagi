    case 0x54: // WORLD's Village of Cainus arrival movie
        // MAIN 06027AA4's slot54 resolves to06027C7A. The native handler
        // updates these MSB-first story bits, then requests55/TWN_CAMP.
        mainGameState.setBit(4 * 8 + 7);
        mainGameState.clearBit(13 * 8 + 2);
        mainGameState.setBit(13 * 8 + 4);
        mainGameState.setBit(13 * 8 + 3);
        setNextGameStatus(0x55);
        break;

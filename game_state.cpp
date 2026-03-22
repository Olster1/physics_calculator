void initFont(GameState *gameState) {
    gameState->mainFont = initFontAtlas(global_Roboto_Regular_ttf);
}

void initGameState(GameState *gameState) {
    gameState->initialized = true;
    loadAnimations(gameState);
    loadImages(gameState);
    initFont(gameState);
    initSound(gameState);
    gameState->grabbedSettings = -1;

    LoadSettingsFileResult loadResult = loadSettingsFile(&gameState->soundAssets, &gameState->settingsToSave);
    gameState->settingsToSave = loadResult.settingsToSave;
    platform_setWindowSize(gameState->settingsToSave.windowX, gameState->settingsToSave.windowY);
    platform_setWindowPos(gameState->settingsToSave.windowPosX, gameState->settingsToSave.windowPosY);
    globalSoundState->channelVolumes[SOUND_CHANNEL_BG] = gameState->settingsToSave.volumeBg;
    globalSoundState->channelVolumes[SOUND_CHANNEL_FG] = gameState->settingsToSave.volumeFg;

    if(loadResult.playingFileId) {
        gameState->boardState = loadResult.state;
        gameState->gameModeState = GAME_MODE_PLAY;
    } else {
        gameState->gameModeState = GAME_START_SCREEN_MODE;
    }

    gameState->backgroundImage = gameState->imageFiles.backgrounds[random_between_int(0, gameState->backgroundImageCount)];

    for(int i = 0; i < arrayCount(gameState->uiFlyInTimers); ++i) {
        gameState->uiFlyInTimers[i].max = random_between_float(MAX_FADE_TIME - 0.2f, MAX_FADE_TIME + 0.1f);
    }
    gameState->customBoardSize = 3;
    gameState->operations = initResizeArray(VmOperation);
    gameState->codeToRun = "";
    gameState->colorPallettes = init_color_palettes();
    gameState->colorPallette = &gameState->colorPallettes.handmade;
}

GameState *allocateGameState() {
    GameState * result = pushStruct(&globalLongTermArena, GameState);
    initGameState(result);
    return result;
    
}
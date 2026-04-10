void initFont(GameState *gameState) {
    gameState->mainFont = initFontAtlas(global_Roboto_Regular_ttf);
}

void initGameState(GameState *gameState) {
    gameState->initialized = true;
    initFont(gameState);
    loadImages(gameState);

    LoadSettingsFileResult loadResult = loadSettingsFile(&gameState->settingsToSave);
    gameState->settingsToSave = loadResult.settingsToSave;
    platform_setWindowSize(gameState->settingsToSave.windowX, gameState->settingsToSave.windowY);
    platform_setWindowPos(gameState->settingsToSave.windowPosX, gameState->settingsToSave.windowPosY);

    gameState->operations = List<VmOperation>::init(&globalLongTermArena);
    gameState->codeToRun = "";
    gameState->colorPallettes = init_color_palettes();
    gameState->colorPallette = &gameState->colorPallettes.witness;
    stringBuffer_init(&gameState->stringBuffer);
    gameState->useRadians = true;
    // DEBUG_MapTests(&globalPerFrameArena);
    // runLanguageUnitTests(gameState);
    gameState->bufferHistory = List<char *>::init(&globalLongTermArena);
    gameState->startUseRadians = gameState->useRadians = true;
}

GameState *allocateGameState() {
    GameState * result = pushStruct(&globalLongTermArena, GameState);
    initGameState(result);
    return result;

}
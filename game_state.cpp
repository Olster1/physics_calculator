void initFont(GameState *gameState) {
    gameState->mainFont = initFontAtlas(global_Roboto_Regular_ttf);
}

void runCalculator(GameState *gameState, bool addSemiColor = true) {

    char *codeToRun = easy_createString_printf(&globalPerClearSessionArena, "%s%s",  gameState->settingsToSave.code, gameState->stringBuffer.string);
    if(addSemiColor) {
        codeToRun = easy_createString_printf(&globalPerClearSessionArena, "%s%s",  codeToRun, "\n");
    }

    gameState->currentCompilerError = 0;

    char *error = compileToByteCode(codeToRun, &gameState->operations, &gameState->calculatorLinesParent);

    if(!error) {
        VmMachineState machineState  = initVmMachineState(gameState->settingsToSave.startUseRadians);
        runCode(&machineState, gameState, &gameState->operations);
        gameState->settingsToSave.code = codeToRun;
        //NOTE: Add string to history
        gameState->bufferHistory.push(easy_createString_printf(&globalLongTermArena, "%s", gameState->stringBuffer.string));
        gameState->historyAt = gameState->bufferHistory.count;
        clearStringBuffer(&gameState->stringBuffer);
        saveSettingsFile(&gameState->settingsToSave);
    } else {
        gameState->currentCompilerError = easy_createString_printf(&globalPerVmRunLifetime, "%s", error);
    }
}

void initGameState(GameState *gameState, BackendRenderer *backendRenderer) {
    gameState->initialized = true;

    gameState->renderer.shaders = &backendRenderer->shaders;

    gameState->textureAtlas = readTextureAtlas("./assets/texture_atlas.json", "./assets/texture_atlas.png");
    initFont(gameState);
    loadImages(&gameState->imageFiles, &gameState->textureAtlas);


    LoadSettingsFileResult loadResult = loadSettingsFile(&gameState->settingsToSave);
    gameState->settingsToSave = loadResult.settingsToSave;
    platform_setWindowSize(gameState->settingsToSave.windowX, gameState->settingsToSave.windowY);
    platform_setWindowPos(gameState->settingsToSave.windowPosX, gameState->settingsToSave.windowPosY);

    gameState->colorPallettes = init_color_palettes();
    gameState->colorPallette = &gameState->colorPallettes.pallettes[gameState->settingsToSave.themeIndex];
    stringBuffer_init(&gameState->stringBuffer);

    gameState->bufferHistory = List<char *>::init(&globalLongTermArena);

    //NOTE: Run the calculator if there was code save from previous session
    if(easyString_getSizeInBytes_utf8(gameState->settingsToSave.code) > 0) {
        runCalculator(gameState, false);
    }


#if UNIT_TESTS_ON
    DEBUG_MapTests(&globalPerFrameArena);
    runLanguageUnitTests(gameState);
#endif

}

GameState *allocateGameState(BackendRenderer *backendRenderer) {
    GameState * result = pushStruct(&globalLongTermArena, GameState);
    initGameState(result, backendRenderer);
    return result;

}
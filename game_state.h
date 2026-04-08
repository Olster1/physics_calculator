enum GameModeState {
	GAME_MODE_STATE_NONE,
	GAME_START_SCREEN_MODE,
	GAME_MAP_MODE,
    GAME_MODE_PLAY,
    GAME_CUSTOM_BOARD_MODE,
    GAME_SETTINGS_MODE
};

void getUiFlyInRange(GameModeState state, int *min, int *max) {
    if(state == GAME_CUSTOM_BOARD_MODE) {
        *min = 1*FLY_IN_TIMERS_PRE_ROW;
        *max = 2*FLY_IN_TIMERS_PRE_ROW;
    } else if(state == GAME_SETTINGS_MODE) {
        *min = 2*FLY_IN_TIMERS_PRE_ROW;
        *max = 3*FLY_IN_TIMERS_PRE_ROW;
    } else {
        *min = 0;
        *max = 1*FLY_IN_TIMERS_PRE_ROW;
    }
}

u32 getUiFlyInId(GameModeState state, u32 localIndex) {
    u32 result = localIndex;
    int pitch = 0;

    if(state == GAME_CUSTOM_BOARD_MODE) {
        pitch = 1;
    } else if(state == GAME_SETTINGS_MODE) {
        pitch = 2;
    }

    result = localIndex + (FLY_IN_TIMERS_PRE_ROW*pitch);

    assert(result < MAX_UI_FLY_IN_TIMERS);

    return result;
}

struct UiFlyInTimer {
    float value;
    float max;
    bool reverse;
};

struct GameState {
    bool initialized;

    float aspectRatioWindow_y_over_x;
    Renderer renderer;

    BoardState boardState;
    SoundAssets soundAssets;

    Texture *backgroundImage;
    int backgroundImageCount;

    float2 scrollWheelDelta;

    List<VmOperation> operations;

    EasyUi_State uiState;

    StringBuffer stringBuffer;
    bool enterPressed;

    StringBuffer *strBufferFreeList;
    float2 bufferOffset;

    int calculatorLineCount;
    int maxCalculatorLineCount;
    CalculatorLine *calculatorLines; //NOTE: Lives on per vm run arena
    char *codeToRun;

    UiFlyInTimer uiFlyInTimers[MAX_UI_FLY_IN_TIMERS];
    bool customMathSymbolsNonActive[4];
    int customBoardSize;
    bool customBoardSizeOpen;
    float dropDownOffset;
    bool dropDownWasOpen;

    GameModeState gameModeState;
	GameModeState targetGameMode;
	float gameModeFadeTimer;
	int gameModeFadeDirection;

    SettingsToSave settingsToSave;
    int grabbedSettings;
    PlayingSound *settingsSound;

    PlayingSound *pullbackSound;

    float2 mouseP_screenSpace;
    float2 mouseP_01;
    MouseKeyState mouseBtn[MOUSE_BUTTON_TYPE_COUNT];

    PlatformAudioSpec audioSpec;

    float dt;
    Font mainFont;

    AnimationState animationState;
    DefaultEntityAnimations playerAnimations;
    ImageFiles imageFiles;

    bool isDraggingCamera;
    float2 cameraDragStart;
    Camera camera;

    Color_Palettes colorPallettes;
    Editor_Color_Palette *colorPallette;
};


void clearCalculatorBuffer(GameState *gameState) {
    refreshVmMemoryArena();
    gameState->operations.clear();
    gameState->calculatorLineCount = 0;

    //TODO: Memory leak if we clear the whole cacluator, use another lifetime arena when I allocate it - not the long term arena
    gameState->codeToRun = 0;
}
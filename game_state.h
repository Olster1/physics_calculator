struct ImageFiles {
    Texture *whiteImage;
};

enum InteractionMode {
    INTERACTION_MODE_DEFAULT,
    INTERACTION_MODE_PICK_THEME,
};

struct GameState {
    bool initialized;

    InteractionMode mode;

    float aspectRatioWindow_y_over_x;
    Renderer renderer;

    float2 scrollWheelDelta;
    char *currentCompilerError;

    List<VmOperation> operations;
    int historyAt;
    List<char *> bufferHistory; //NOTE: Strings users have entered already which last the lifetime of the program

    EasyUi_State uiState;

    StringBuffer stringBuffer;
    bool enterPressed;

    StringBuffer *strBufferFreeList;
    float2 bufferOffset;

    CalculatorLines calculatorLinesParent;

    SettingsToSave settingsToSave;

    float2 mouseP_screenSpace;
    float2 mouseP_01;
    MouseKeyState mouseBtn[MOUSE_BUTTON_TYPE_COUNT];


    float dt;
    Font mainFont;

    ImageFiles imageFiles;

    Camera camera;

    Color_Palettes colorPallettes;
    Editor_Color_Palette *colorPallette;
};


void clearCalculatorBuffer(GameState *gameState) {
    refreshVmMemoryArena();
    refreshPerClearSessionArena();
    gameState->operations.clear();
    gameState->calculatorLinesParent.calculatorLineCount = 0;
    gameState->settingsToSave.startUseRadians = gameState->settingsToSave.useRadians;
    //NOTE: This string is stored int the per clear session arena so is free now
    gameState->settingsToSave.code = "";

    saveSettingsFile(&gameState->settingsToSave);
}
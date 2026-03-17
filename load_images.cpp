void loadAnimations(GameState *gameState) {
}

void loadImages(GameState *gameState) {
    gameState->imageFiles.whiteImage = platform_loadImage("./assets/white.png", &globalLongTermArena);
    global_white_image = gameState->imageFiles.whiteImage;

    gameState->imageFiles.textBackdrop = platform_loadImage("./assets/outlineBorder.png", &globalLongTermArena);
    gameState->imageFiles.circleOutline = platform_loadImage("./assets/circleOutline.png", &globalLongTermArena);
    gameState->imageFiles.circleBackDrop = platform_loadImage("./assets/circleBackDrop.png", &globalLongTermArena);
    gameState->imageFiles.cancelSelection = platform_loadImage("./assets/cancelSelection.png", &globalLongTermArena);
    gameState->imageFiles.eraseSelection = platform_loadImage("./assets/eraseSelection.png", &globalLongTermArena);
    gameState->imageFiles.undo = platform_loadImage("./assets/undo.png", &globalLongTermArena);
    gameState->imageFiles.redo = platform_loadImage("./assets/redo.png", &globalLongTermArena);

    gameState->imageFiles.outlineBottomEnd = platform_loadImage("./assets/end_bottom.png", &globalLongTermArena);
    gameState->imageFiles.outlineTopEnd = platform_loadImage("./assets/end_top.png", &globalLongTermArena);
    gameState->imageFiles.outlineLeftEnd = platform_loadImage("./assets/end_left.png", &globalLongTermArena);
    gameState->imageFiles.outlineRightEnd = platform_loadImage("./assets/end_right.png", &globalLongTermArena);

    gameState->imageFiles.outlineCornerBottomLeft = platform_loadImage("./assets/corner_bottom_left.png", &globalLongTermArena);
    gameState->imageFiles.outlineCornerTopLeft = platform_loadImage("./assets/corner_top_left.png", &globalLongTermArena);
    gameState->imageFiles.outlineCornerBottomRight = platform_loadImage("./assets/corner_bottom_right.png", &globalLongTermArena);
    gameState->imageFiles.outlineCornerTopRight = platform_loadImage("./assets/corner_top_right.png", &globalLongTermArena);

    gameState->imageFiles.topLeftRight = platform_loadImage("./assets/top_left_right.png", &globalLongTermArena);
    gameState->imageFiles.bottomLeftRight = platform_loadImage("./assets/bottom_left_right.png", &globalLongTermArena);
    gameState->imageFiles.topBottomRight = platform_loadImage("./assets/top_bottom_right.png", &globalLongTermArena);
    gameState->imageFiles.topBottomLeft = platform_loadImage("./assets/top_bottom_left.png", &globalLongTermArena);

    gameState->imageFiles.outlineTwoAcross = platform_loadImage("./assets/2_across.png", &globalLongTermArena);
    gameState->imageFiles.outlineTwoUp = platform_loadImage("./assets/2_up.png", &globalLongTermArena);
    
    gameState->imageFiles.outlineAllDirections = platform_loadImage("./assets/all_directions.png", &globalLongTermArena);
    gameState->imageFiles.outlineSingle = platform_loadImage("./assets/1_outline.png", &globalLongTermArena);
    gameState->imageFiles.roundedRect = platform_loadImage("./assets/roundedRect.png", &globalLongTermArena);
    gameState->imageFiles.circle = platform_loadImage("./assets/circle.png", &globalLongTermArena);

    gameState->imageFiles.add = platform_loadImage("./assets/ui/add.png" , &globalLongTermArena);
    gameState->imageFiles.back = platform_loadImage("./assets/ui/back.png", &globalLongTermArena);
    gameState->imageFiles.back1 = platform_loadImage("./assets/ui/back1.png", &globalLongTermArena);
    gameState->imageFiles.back2 = platform_loadImage("./assets/ui/back2.png", &globalLongTermArena);
    gameState->imageFiles.back3 = platform_loadImage("./assets/ui/back3.png", &globalLongTermArena);
    gameState->imageFiles.back4 = platform_loadImage("./assets/ui/back4.png", &globalLongTermArena);
    gameState->imageFiles.divide = platform_loadImage("./assets/ui/divide.png", &globalLongTermArena);
    gameState->imageFiles.hint = platform_loadImage("./assets/ui/hint.png", &globalLongTermArena);
    gameState->imageFiles.light = platform_loadImage("./assets/ui/light.png", &globalLongTermArena);
    gameState->imageFiles.minus = platform_loadImage("./assets/ui/minus.png", &globalLongTermArena);
    gameState->imageFiles.multiply = platform_loadImage("./assets/ui/multiply.png", &globalLongTermArena);
    gameState->imageFiles.redo = platform_loadImage("./assets/ui/redo.png", &globalLongTermArena);
    gameState->imageFiles.refresh = platform_loadImage("./assets/ui/refresh.png", &globalLongTermArena);
    gameState->imageFiles.undo = platform_loadImage("./assets/ui/undo.png", &globalLongTermArena);
    gameState->imageFiles.volume = platform_loadImage("./assets/ui/volume.png", &globalLongTermArena);
    gameState->imageFiles.settings = platform_loadImage("./assets/ui/settings.png", &globalLongTermArena);
    gameState->imageFiles.numberBg = platform_loadImage("./assets/ui/numberBg.png", &globalLongTermArena);

    gameState->imageFiles.backgrounds[gameState->backgroundImageCount++] = platform_loadImage("./assets/backgrounds/japan_background.png", &globalLongTermArena);
    gameState->imageFiles.backgrounds[gameState->backgroundImageCount++] = platform_loadImage("./assets/backgrounds/beach_background.png", &globalLongTermArena);
    gameState->imageFiles.backgrounds[gameState->backgroundImageCount++] = platform_loadImage("./assets/backgrounds/forest_background.png", &globalLongTermArena);
    gameState->imageFiles.backgrounds[gameState->backgroundImageCount++] = platform_loadImage("./assets/backgrounds/village_background.png", &globalLongTermArena);
    gameState->imageFiles.backgrounds[gameState->backgroundImageCount++] = platform_loadImage("./assets/backgrounds/tuscany_background.png", &globalLongTermArena);
    assert(gameState->backgroundImageCount < MAX_BACKGROUND_IMAGES);
}
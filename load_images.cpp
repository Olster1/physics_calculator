void loadImages(GameState *gameState) {
    gameState->imageFiles.whiteImage = platform_loadImage("./assets/white.png", &globalLongTermArena);
    global_white_image = gameState->imageFiles.whiteImage;
}
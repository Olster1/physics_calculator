struct ImageFiles {
    Texture *whiteImage;
    Texture *test;
};

void loadImages(ImageFiles *imageFiles, TextureAtlas *textureAtlas) {
    // imageFiles->whiteImage = platform_loadImage("./assets/images/white.png", &globalLongTermArena);
    imageFiles->whiteImage = textureAtlas_getItemAsTexture(textureAtlas, "white.png", &globalLongTermArena);
    imageFiles->test = textureAtlas_getItemAsTexture(textureAtlas, "logo_apple.png", &globalLongTermArena);
    global_white_image = imageFiles->whiteImage;
}
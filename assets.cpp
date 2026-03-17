
static void loadImageStrip(Animation *animation, char *filename_full_utf8, int widthPerImage) {
    if(platformDoesFileExist(filename_full_utf8)) {
        Texture *texOnStack = platform_loadImage(filename_full_utf8, &globalLongTermArena);
        int count = 0;

        float xAt = 0;

        float widthTruncated = ((int)(texOnStack->width / widthPerImage))*widthPerImage;
        while(xAt < widthTruncated) {
            Texture *tex = pushStruct(&globalLongTermArena, Texture);
            easyPlatform_copyMemory(tex, texOnStack, sizeof(Texture));

            tex->uv.x = xAt / texOnStack->width;

            xAt += widthPerImage;

            tex->uv.z = xAt / texOnStack->width;

            easyAnimation_pushFrame(animation, tex);

            count++;
        }
    } else {
        assert(false);
    }
}
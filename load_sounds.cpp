
void initSound(GameState *gameState) {
    initAudioSpec(&gameState->audioSpec, AUDIO_SAMPLE_RATE);
    initAudio(&gameState->audioSpec, AUDIO_SAMPLE_RATE);
    int musicCount = 0;
    loadOggVorbisFile(&gameState->soundAssets.backgroundMusic[musicCount++], "./assets/sounds/music1.ogg", &gameState->audioSpec);
    loadOggVorbisFile(&gameState->soundAssets.backgroundMusic[musicCount++], "./assets/sounds/music2.ogg", &gameState->audioSpec);
    loadOggVorbisFile(&gameState->soundAssets.backgroundMusic[musicCount++], "./assets/sounds/music3.ogg", &gameState->audioSpec);
    loadOggVorbisFile(&gameState->soundAssets.backgroundMusic[musicCount++], "./assets/sounds/music4.ogg", &gameState->audioSpec);
    gameState->soundAssets.backgroundSoundCount = musicCount;

    loadOggVorbisFile(&gameState->soundAssets.eraseSound, "./assets/sounds/erase.ogg", &gameState->audioSpec);
    loadOggVorbisFile(&gameState->soundAssets.placeSound, "./assets/sounds/place.ogg", &gameState->audioSpec);

    loadOggVorbisFile(&gameState->soundAssets.ambience, "./assets/sounds/ambience.ogg", &gameState->audioSpec);

    loadOggVorbisFile(&gameState->soundAssets.introSound, "./assets/sounds/intro_sound.ogg", &gameState->audioSpec);

    int indexCount = gameState->soundAssets.backgroundSoundCount;
    int *indexes = pushArray(&globalPerFrameArena, gameState->soundAssets.backgroundSoundCount, int);

    for(int i = 0; i < indexCount; ++i) {
        indexes[i] = i;
    }

    PlayingSound *lastSound = 0;
    PlayingSound *firstSound = 0;
    while(indexCount > 0) {
        int index = random_between_int(0, indexCount);
        int soundIndex = indexes[index];

        PlayingSound *sound = 0;
        if(!firstSound) {
            sound = playSound(&gameState->soundAssets.backgroundMusic[soundIndex]);
            firstSound = sound;
        } else {
            sound = getPlaySound(&gameState->soundAssets.backgroundMusic[soundIndex]);
        }

        PlayingSound *ambienceSound = getPlaySound(&gameState->soundAssets.ambience);
        ambienceSound->volume = 2;

        sound->nextSound = ambienceSound;

        if(lastSound) {
            lastSound->nextSound = sound;
        }

        lastSound = ambienceSound;

        //NOTE: Replace the index
        indexes[index] = indexes[--indexCount];
    }

    lastSound->nextSound = firstSound;

    assert(musicCount < MAX_BACKGROUND_SOUNDS);
}
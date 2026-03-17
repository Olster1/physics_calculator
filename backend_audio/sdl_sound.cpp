
struct PlatformAudioSpec{
    SDL_AudioSpec audioSpec;
    SDL_AudioStream *stream;
};

void loadOggVorbisFile(GameSoundAsset *result, char *fileName, PlatformAudioSpec *audioSpecWrapper) {
    result->fileName = fileName;
}

#define initAudioSpec(audioSpec, frequency) initAudioSpec_(audioSpec, frequency)

void initAudioSpec_(PlatformAudioSpec *audioSpecWrapper, int frequency) {
    SDL_AudioSpec *audioSpec = &audioSpecWrapper->audioSpec;
    /* Set the audio format */
    audioSpec->freq = frequency;
    audioSpec->format = SDL_AUDIO_S16LE;
    audioSpec->channels = AUDIO_STEREO;
    // audioSpec->callback = callback;
    // assert(callback);
}

bool initAudio(PlatformAudioSpec *audioSpecWrapper, int frequency) {
    SDL_AudioSpec *audioSpec = &audioSpecWrapper->audioSpec;
    bool successful = true;
    globalSoundState = pushStruct(&globalLongTermArena, EasySound_SoundState);

    initGlobalSoundState(globalSoundState);
    

    SDL_AudioDeviceID device = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, audioSpec);
    /* Open the audio device, forcing the desired format */
    if (device == 0) {
        fprintf(stderr, "Couldn't open audio: %s\n", SDL_GetError());
        successful = false;
    } else {
        // SDL_PauseAudioDevice(device);
    }

    SDL_AudioSpec source = {SDL_AUDIO_S16LE, AUDIO_STEREO, frequency};
    SDL_AudioSpec dest = {SDL_AUDIO_S16LE, AUDIO_STEREO, frequency};

    audioSpecWrapper->stream = SDL_CreateAudioStream(
        &source,
        &dest
    );

    SDL_BindAudioStream(device, audioSpecWrapper->stream);
    SDL_ResumeAudioDevice(device);

    return successful;
}


void easyAudio_mixAudio_internal(u8 *stream, int bytesToWrite) {
    int bytes_per_sample = sizeof(s16); // AUDIO_S16
    int totalSamples = bytesToWrite / bytes_per_sample; 

    //NOTE: This might be on a seperate thread so we can't use a temporary memory arena.
    //      Could have a seperate one for this thread
    float *mixerBuffer = (float *)malloc(totalSamples*sizeof(float));
    SDL_memset(mixerBuffer, 0, totalSamples*sizeof(float));
    s16 *oggBuffer = (s16 *)malloc(totalSamples*sizeof(s16));

    PlayingSound **soundPrt = &globalSoundState->playingSounds;
    while(*soundPrt) {
        bool advancePtr = true;
        PlayingSound *sound = *soundPrt;

        int samples = stb_vorbis_get_samples_short_interleaved(sound->stream, AUDIO_STEREO, oggBuffer, totalSamples);
        samples *= AUDIO_STEREO;

        float soundVolume = globalSoundState->channelVolumes[sound->soundChannelType]*sound->volume;

        //NOTE: This is the Audio Mixer - it sould do everything in float then sample down
        for(int i = 0; i < samples; i++) {
            float a = mixerBuffer[i];
            float value = (float)(((s16 *)oggBuffer)[i]);
            // assert(sound->volume >= 0 && sound->volume <= 1);
            float b = soundVolume*value;
            mixerBuffer[i] = a + b;
        }

    
        if(samples == 0) {
            if(sound->nextSound) {
                //TODO: Allow the remaining bytes to loop back round and finish the full duration 
                stb_vorbis_close(sound->stream);
                *soundPrt = sound->nextSound;
                sound = *soundPrt;

                easyAudio_startOggStream(sound);
            } else {
                stb_vorbis_close(sound->stream);
                //remove from linked list
                advancePtr = false;
                *soundPrt = sound->next;
                sound->next = globalSoundState->playingSoundsFreeList;
                globalSoundState->playingSoundsFreeList = sound;
            }
        }

        if(sound->shouldEnd) {
            advancePtr = false;

            *soundPrt = sound->next;

            PlayingSound *startSound = sound;

            //NOTE: If sound as a list of next sounds, put all these on the free list
            while(sound) {
                stb_vorbis_close(sound->stream);
                sound->next = globalSoundState->playingSoundsFreeList;
                globalSoundState->playingSoundsFreeList = sound;

                sound = sound->nextSound;
                //NOTE: Prevent looped sounds going in an infinte loop
                if(sound == startSound) {
                    sound = 0;
                }
            }
            
        }
        
        if(advancePtr) {
            soundPrt = &((*soundPrt)->next);
        }
    }

    s16 *s = (s16 *)stream;
    for(int i = 0; i < totalSamples; ++i) {
        int sample = (int)(mixerBuffer[i]);
        if (sample > 32767) sample = 32767;
        else if (sample < -32768) sample = -32768;
        s[i] = (s16)sample;
    }

    free(mixerBuffer);
    free(oggBuffer);
}

void easyAudio_mixAudio(SDL_AudioStream *stream) {
    int bytes_per_sample = sizeof(s16) * 2; // stereo
    int target_bytes =
        (AUDIO_SAMPLE_RATE * TARGET_LATENCY_MS / 1000) * bytes_per_sample;

    int availableBytes = SDL_GetAudioStreamAvailable(stream);

    if (availableBytes < target_bytes) {
        
        int bytes_to_write = target_bytes - availableBytes;
        u8 *buffer = pushArray(&globalPerFrameArena, bytes_to_write, u8);
        easyAudio_mixAudio_internal(buffer, bytes_to_write);

        SDL_PutAudioStreamData(stream, buffer, bytes_to_write);
    }

}

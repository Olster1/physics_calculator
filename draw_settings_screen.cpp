void drawSettingsScreen(GameState *gameState, float2 plane, float2 mouseWorldP) {
    GameModeState gameModeState = GAME_SETTINGS_MODE;
    float smallestDim = plane.x;
    float divisor = 3.0;
    if(plane.y < plane.x) {
        smallestDim = plane.y;
        divisor = 5.0;
    }

    float btnSize = smallestDim / divisor;
    int buttonPerRow = 1;
    float buttonPerColumn = 2;

    float2 cursorAt = make_float2(-0.5f*(buttonPerRow*btnSize) + 0.5f*btnSize, 0.5f*(buttonPerColumn*btnSize) - 0.5f*btnSize);
    float2 startCursorAt = cursorAt;
    int boardType = 3;
    int localIndex = 0;
    float values[2] = {gameState->settingsToSave.volumeBg, gameState->settingsToSave.volumeFg, };
    char *labels[2] = {"Background Volume", "Foreground Volume"};
    for(int j = 0; j < (int)buttonPerColumn; ++j) {
        for(int i = 0; i < buttonPerRow; ++i) {
            float sliderWidth = 0.6f*plane.x;
            float startX = -0.5f*sliderWidth;
            
            float3 pos = make_float3(values[j]*sliderWidth + startX, cursorAt.y, 1);
            float alpha = 1;
            
            updateFlyInTimer(gameState, &pos.x, &alpha, localIndex, pos.xy, plane, gameModeState);

            pushRenderTexture(&gameState->renderer, make_transformX(make_float3(0, cursorAt.y, 1), make_float3(sliderWidth, 2, 1), make_float4(0, 0, 0, 1)), gameState->imageFiles.whiteImage, make_float4(0.8f, 0.8f, 0.8f, alpha));
            pushRenderTexture(&gameState->renderer, make_transformX(make_float3(startX + 0.5f*(values[j]*sliderWidth), cursorAt.y, 1), make_float3(values[j]*sliderWidth, 2, 1), make_float4(0, 0, 0, 1)), gameState->imageFiles.whiteImage, make_float4(1, 1, 1, alpha));
            // pushRenderTexture(&gameState->renderer, make_transformX(pos, make_float3(0.6f*plane.x, 2, 1), make_float4(0, 0, 0, 1)), gameState->imageFiles.whiteImage, make_float4(0.2f, 0.2f, 0.2f, alpha));
            pushRenderTexture(&gameState->renderer, make_transformX(pos, make_float3(4, 4, 1), make_float4(0, 0, 0, 1)), gameState->imageFiles.circle, make_float4(0.9f, 0.9f, 0.9f, alpha));
            renderText_centered(&gameState->renderer, &gameState->mainFont, labels[j], make_float2(0, cursorAt.y + 3.2f), 0.8f*BODY_FONT_SCALE, make_float4(1, 1, 1, alpha));

            Rect2f bounds = make_rect2f_center_dim(pos.xy, make_float2(btnSize, btnSize));
            if(!isGameTransitioningScene(gameState) && in_rect2f_bounds(bounds, mouseWorldP)) {
                if(gameState->mouseBtn[MOUSE_BUTTON_LEFT_CLICK] == MOUSE_BUTTON_PRESSED) {
                  gameState->grabbedSettings = j;
                }
            }

            if(gameState->grabbedSettings == j) {
                float value = mouseWorldP.x - startX;

                float value_01 = value / sliderWidth;

                value_01 = clamp(0, 1, value_01);
                
                if(j == 0) {
                    globalSoundState->channelVolumes[SOUND_CHANNEL_BG] = gameState->settingsToSave.volumeBg = value_01;
                } else {
                    globalSoundState->channelVolumes[SOUND_CHANNEL_FG] = gameState->settingsToSave.volumeFg = value_01; 

                    if(!gameState->settingsSound) {
                        gameState->settingsSound = playSoundForeground(&gameState->soundAssets.placeSound);
                        easySound_loopSound(gameState->settingsSound);
                    }
                }
                saveSettingsFile(&gameState->settingsToSave, 0);
            }

            cursorAt.x += btnSize;
            boardType++;
            localIndex++;
        }
        cursorAt.x = startCursorAt.x;
        cursorAt.y -= btnSize;
    }
    {
        float btnSize = getBackButtonSize(plane);
        float btnOffset = getBackButtonOffset(plane);
        float3 pos = make_float3(-0.5f*plane.x + btnOffset, 0.5f*plane.y - btnOffset, 1);
        float alpha = 1;
    
        updateFlyInTimer(gameState, &pos.x, &alpha, localIndex++, pos.xy, plane, gameModeState);
        
        float3 btnScale = make_float3(btnSize, btnSize, 1);
        pushRenderTexture(&gameState->renderer, make_transformX(pos, make_float3(2*btnScale.x, 2*btnScale.y, 1)), gameState->imageFiles.back2, make_float4(1, 1, 1, alpha));
        pushRenderTexture(&gameState->renderer, make_transformX(pos, make_float3(btnScale.x, btnScale.y, 1)), gameState->imageFiles.back, make_float4(1, 1, 1, alpha));
        

        Rect2f bounds = make_rect2f_center_dim(pos.xy, scale_float2(2, btnScale.xy));
        if(in_rect2f_bounds(bounds, mouseWorldP) && !isGameTransitioningScene(gameState)) {
            if(gameState->mouseBtn[MOUSE_BUTTON_LEFT_CLICK] == MOUSE_BUTTON_PRESSED) {
                goToGameMode(gameState, GAME_START_SCREEN_MODE);
                resetUiFlyInTimersArray(gameState, true, gameModeState);
                resetUiFlyInTimersArray(gameState, false, GAME_START_SCREEN_MODE);
            }
        }
    }
}
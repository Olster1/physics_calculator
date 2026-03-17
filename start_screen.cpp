void updateFlyInTimer(GameState *gameState, float *posX, float *alpha, int localIndex, float2 targetPos, float2 plane, GameModeState gameModeState) {
    int index = getUiFlyInId(gameModeState, localIndex);
    float flyTimerMax = gameState->uiFlyInTimers[index].max;
    if(gameState->uiFlyInTimers[index].value >= 0) {
        gameState->uiFlyInTimers[index].value += gameState->dt;

        float t = Math3D_smoothstep01(gameState->uiFlyInTimers[index].value / flyTimerMax);

        if(gameState->uiFlyInTimers[index].value >= flyTimerMax) {
            gameState->uiFlyInTimers[index].value = -1;

        } else {
            float a = targetPos.x + 0.5*plane.x;
            float b = targetPos.x;
            *alpha = t;
            if(gameState->uiFlyInTimers[index].reverse) {
                a = targetPos.x;
                b = targetPos.x + 0.7*plane.x;
                *alpha = 1.0f - t;
                
            }
            *posX = lerp(a, b, make_lerpTValue(t));
        }
    } 

    //NOTE: Make sure the position stays in the end position
    if(gameState->uiFlyInTimers[index].value < 0) {
        if(gameState->uiFlyInTimers[index].reverse) {
            *posX = targetPos.x + 0.7*plane.x;
            *alpha = 0;
        } else {
            *posX = targetPos.x;
            *alpha = 1;
        }
    }
}


void drawStartScreen(GameState *gameState, float2 plane, float2 mouseWorldP) {
    GameModeState gameModeState = GAME_START_SCREEN_MODE;
    float smallestDim = plane.x;
    if(plane.y < plane.x) {
        smallestDim = plane.y;
    }

    float btnSize = smallestDim / 5.0f;
    int buttonPerRow = 3;
    float buttonPerColumn = 3.5f;

    float2 cursorAt = make_float2(-0.5f*(buttonPerRow*btnSize) + 0.5f*btnSize, 0.5f*(buttonPerColumn*btnSize) - 0.5f*btnSize);
    float2 startCursorAt = cursorAt;
    int boardType = 3;
    int localIndex = 0;
    for(int j = 0; j < (int)buttonPerColumn; ++j) {
        for(int i = 0; i < buttonPerRow; ++i) {
            
            float3 pos = make_float3(cursorAt.x, cursorAt.y, 1);
            float alpha = 1;
            
            updateFlyInTimer(gameState, &pos.x, &alpha, localIndex, cursorAt, plane, gameModeState);
            pushRenderTexture(&gameState->renderer, make_transformX(pos, make_float3(btnSize, btnSize, 1), make_float4(0, 0, 0, 1)), gameState->imageFiles.roundedRect, make_float4(1, 1, 1, alpha));

            renderText_centered(&gameState->renderer, &gameState->mainFont, easy_createString_printf(&globalPerFrameArena, "%d", boardType, boardType), pos.xy, BODY_FONT_SCALE, make_float4(0, 0, 0, 1));

            Rect2f bounds = make_rect2f_center_dim(pos.xy, make_float2(btnSize, btnSize));
            if(!isGameTransitioningScene(gameState) && in_rect2f_bounds(bounds, mouseWorldP)) {
                if(gameState->mouseBtn[MOUSE_BUTTON_LEFT_CLICK] == MOUSE_BUTTON_PRESSED) {
                    easyMemory_zeroArray(gameState->customMathSymbolsNonActive);
                    gameState->boardState = initBoardState(boardType, &gameState->soundAssets, &gameState->settingsToSave);
                    goToGameMode(gameState, GAME_MODE_PLAY);
                    resetUiFlyInTimersArray(gameState, true, gameModeState);
                    
                }
            }

            cursorAt.x += btnSize;
            boardType++;
            localIndex++;
        }
        cursorAt.x = startCursorAt.x;
        cursorAt.y -= btnSize;
    }
    {
        float3 pos = make_float3(0, cursorAt.y + 0.25f*btnSize, 1);
        float alpha = 1;
    
        updateFlyInTimer(gameState, &pos.x, &alpha, localIndex++, make_float2(0, 0), plane, gameModeState);
        
        float3 btnScale = make_float3(3*btnSize, 0.5f*btnSize, 1);
        pushRenderTexture(&gameState->renderer, make_transformX(pos, btnScale, make_float4(0, 0, 0, 1)), gameState->imageFiles.roundedRect, make_float4(1, 1, 1, alpha));
        renderText_centered(&gameState->renderer, &gameState->mainFont, "Custom", pos.xy, BODY_FONT_SCALE, make_float4(0, 0, 0, 1));

        Rect2f bounds = make_rect2f_center_dim(pos.xy, btnScale.xy);
        if(in_rect2f_bounds(bounds, mouseWorldP) && !isGameTransitioningScene(gameState)) {
            if(gameState->mouseBtn[MOUSE_BUTTON_LEFT_CLICK] == MOUSE_BUTTON_PRESSED) {
                goToGameMode(gameState, GAME_CUSTOM_BOARD_MODE);
                resetUiFlyInTimersArray(gameState, true, gameModeState);
                resetUiFlyInTimersArray(gameState, false, GAME_CUSTOM_BOARD_MODE);
            }
        }
    }

    {
        float backBtnOffset = getBackButtonOffset(plane);
        float backBtnSize = getBackButtonSize(plane);
        float3 btnScale = make_float3(backBtnSize, backBtnSize, 1);

        float3 pos = make_float3(-0.5f*plane.x + backBtnOffset, 0.5f*plane.y - backBtnOffset, 1);
        float alpha = 1;
    
        updateFlyInTimer(gameState, &pos.x, &alpha, localIndex++, pos.xy, plane, gameModeState);
        
        pushRenderTexture(&gameState->renderer, make_transformX(pos, make_float3(2*btnScale.x, 2*btnScale.y, 1), make_float4(0, 0, 0, 1)), gameState->imageFiles.back2, make_float4(1, 1, 1, alpha));
        pushRenderTexture(&gameState->renderer, make_transformX(pos, btnScale), gameState->imageFiles.settings, make_float4(1, 1, 1, alpha));

        Rect2f bounds = make_rect2f_center_dim(pos.xy, scale_float2(2, btnScale.xy));
        if(in_rect2f_bounds(bounds, mouseWorldP) && !isGameTransitioningScene(gameState)) {
            if(gameState->mouseBtn[MOUSE_BUTTON_LEFT_CLICK] == MOUSE_BUTTON_PRESSED) {
                goToGameMode(gameState, GAME_SETTINGS_MODE);
                resetUiFlyInTimersArray(gameState, true, gameModeState);
                resetUiFlyInTimersArray(gameState, false, GAME_SETTINGS_MODE);
            }
        }
    }
}
bool custom_safeToProcessClick(GameState *gameState) {
    return (!isGameTransitioningScene(gameState) && !gameState->customBoardSizeOpen && !gameState->dropDownWasOpen);
}

void drawCustomSettingsScreen(GameState *gameState, float2 plane, float2 mouseWorldP) {
       GameModeState gameModeState = GAME_CUSTOM_BOARD_MODE;
    float smallestDim = plane.x;
    if(plane.y < plane.x) {
        smallestDim = plane.y;
    }

    float btnSize = smallestDim / 5.0f;
    int buttonPerRow = 2;
    float buttonPerColumn = 2.5f;

    float2 cursorAt = make_float2(-0.5f*(buttonPerRow*btnSize) + 0.5f*btnSize, 0.5f*(buttonPerColumn*btnSize) - 0.5f*btnSize);
    float2 startCursorAt = cursorAt;
    int boardType = 3;
    int localIndex = 0;
    char *mathSymbols[] = {"+", "x", "-", "/"};
    Texture *mathSymbolTextures[] = {gameState->imageFiles.add, gameState->imageFiles.multiply, gameState->imageFiles.minus, gameState->imageFiles.divide};

    for(int j = 0; j < (int)buttonPerColumn; ++j) {
        for(int i = 0; i < buttonPerRow; ++i) {
            
            float3 pos = make_float3(cursorAt.x, cursorAt.y, 1);
            float alpha = 1;
            bool isReverse = gameState->uiFlyInTimers[getUiFlyInId(gameModeState, localIndex)].reverse;
            
            updateFlyInTimer(gameState, &pos.x, &alpha, localIndex, cursorAt, plane, gameModeState);

            float4 btnColor = make_float4(1, 1, 1, alpha);
            float4 symbolColor = make_float4(0, 0, 0, alpha);
            
            if(gameState->customMathSymbolsNonActive[localIndex]) {
                float a = 1.0f;
                float b = 0.4f;

                if(isReverse) {
                    a = 0;
                }
                btnColor.w = lerp(a, b, make_lerpTValue(alpha));
                symbolColor.w = lerp(a, b, make_lerpTValue(alpha));
            }

            pushRenderTexture(&gameState->renderer, make_transformX(pos, make_float3(btnSize, btnSize, 1), make_float4(0, 0, 0, 1)), gameState->imageFiles.roundedRect, btnColor);
            pushRenderTexture(&gameState->renderer, make_transformX(pos, make_float3(0.3f*btnSize, 0.3f*btnSize, 1), make_float4(1, 1, 1, 1)), mathSymbolTextures[localIndex], symbolColor);

            Rect2f bounds = make_rect2f_center_dim(pos.xy, make_float2(btnSize, btnSize));
            if(custom_safeToProcessClick(gameState) && in_rect2f_bounds(bounds, mouseWorldP)) {
                if(gameState->mouseBtn[MOUSE_BUTTON_LEFT_CLICK] == MOUSE_BUTTON_PRESSED) {
                    int count = 0;
                    for(int i = 0; i < arrayCount(gameState->customMathSymbolsNonActive); ++i) {
                        if(!gameState->customMathSymbolsNonActive[i]) {
                            count++;
                        }
                    }

                    bool value = !gameState->customMathSymbolsNonActive[localIndex];
                    if(count > 1 || !value) {
                        gameState->customMathSymbolsNonActive[localIndex] = value;
                        if(!value) {
                            playSoundForeground(&gameState->soundAssets.placeSound);
                        } else {
                            playSoundForeground(&gameState->soundAssets.eraseSound);
                        }
                        
                    }
                }
            }

            cursorAt.x += btnSize;
            boardType++;
            localIndex++;
        }
        cursorAt.x = startCursorAt.x;
        cursorAt.y -= btnSize;
    }
    

    Renderer *tempRenderer = pushStruct(&globalPerFrameArena, Renderer);
    //NOTE: Size dropdown
    {
        {
            float3 pos = make_float3(-0.5f*btnSize, cursorAt.y + 0.25f*btnSize, 1);
            float alpha = 1;
           
            updateFlyInTimer(gameState, &pos.x, &alpha, localIndex++, pos.xy, plane, gameModeState);
            float3 btnScale = make_float3(btnSize, 0.5f*btnSize, 1);
            pushRenderTexture(tempRenderer, make_transformX(pos, btnScale, make_float4(0, 0, 0, 1)), gameState->imageFiles.roundedRect, make_float4(0.5f, 0.5f, 0.5f, alpha));
            renderText_centered(tempRenderer, &gameState->mainFont, "Size", pos.xy, BODY_FONT_SCALE, make_float4(0, 0, 0, 1));

            gameState->dropDownWasOpen = gameState->customBoardSizeOpen;
        }

        //NOTE: The Dropdown selector
        {
            float3 pos = make_float3(0.5f*btnSize, cursorAt.y + 0.25f*btnSize, 1);
            float alpha = 1;
            
            updateFlyInTimer(gameState, &pos.x, &alpha, localIndex++, pos.xy, plane, gameModeState);
            float3 btnScale = make_float3(btnSize, 0.5f*btnSize, 1);

            localIndex++;
            pushRenderTexture(tempRenderer, make_transformX(pos, btnScale, make_float4(0, 0, 0, 1)), gameState->imageFiles.roundedRect, make_float4(1, 1, 1, alpha));
            renderText_centered(tempRenderer, &gameState->mainFont, easy_createString_printf(&globalPerFrameArena, "%d", gameState->customBoardSize), pos.xy, BODY_FONT_SCALE, make_float4(0, 0, 0, 1));

            if(gameState->customBoardSizeOpen) {
                float cellSize = smallestDim / 8.0f;
                pushRenderTexture(tempRenderer, make_transformX(make_float3(0, 0, 1), make_float3(plane.x, plane.y, 1), make_float4(0, 0, 0, 1)), gameState->imageFiles.whiteImage, make_float4(0, 0, 0, 0.5));
                float2 p = pos.xy;
                gameState->dropDownOffset -= gameState->scrollWheelDelta.y;
                if(gameState->dropDownOffset > 0) {
                    gameState->dropDownOffset = 0;
                }   
                float max = -6*cellSize;
                if(gameState->dropDownOffset < max) {
                    gameState->dropDownOffset = max;
                }
                p.y += gameState->dropDownOffset;
                
                float fontScale = BODY_FONT_SCALE;
                
                for(int i = 3; i < 12; i++) {
                    float2 btnScale = renderText_getDim(tempRenderer, &gameState->mainFont, easy_createString_printf(&globalPerFrameArena, "%d", i), fontScale);

                    Rect2f bounds = make_rect2f_center_dim(p, make_float2(cellSize, cellSize));
                    float4 higlightColor = make_float4(1, 1, 1, 1);

                    if(in_rect2f_bounds(bounds, mouseWorldP) && !isGameTransitioningScene(gameState)) {
                        if(gameState->mouseBtn[MOUSE_BUTTON_LEFT_CLICK] == MOUSE_BUTTON_PRESSED) {
                            gameState->customBoardSize = i;
                        }
                        higlightColor = make_float4(1, 1, 0, 1);
                    }
                    pushRenderTexture(tempRenderer, make_transformX(make_float3(p.x, p.y, 1), make_float3(cellSize, cellSize, 1), make_float4(0, 0, 0, 1)), gameState->imageFiles.roundedRect, higlightColor);
                    renderText_centered(tempRenderer, &gameState->mainFont, easy_createString_printf(&globalPerFrameArena, "%d", i), p, fontScale, make_float4(0, 0, 0, 1));
                    p.y += cellSize;
                }
            }
            Rect2f bounds = make_rect2f_center_dim(pos.xy, btnScale.xy);
            if(gameState->mouseBtn[MOUSE_BUTTON_LEFT_CLICK] == MOUSE_BUTTON_PRESSED) {
                if(in_rect2f_bounds(bounds, mouseWorldP) && !isGameTransitioningScene(gameState) && !gameState->customBoardSizeOpen) {
                    gameState->customBoardSizeOpen = true;
                } else {
                    gameState->customBoardSizeOpen = false;
                }
            } 
            
        }
        cursorAt.y -= 0.5f*btnSize;
    }
    {
        float3 pos = make_float3(0, cursorAt.y + 0.25f*btnSize, 1);
        float alpha = 1;
        if(gameState->uiFlyInTimers[getUiFlyInId(gameModeState, localIndex)].reverse) {
            pos.x = 0.7*plane.x;
            alpha = 0;
        }
        updateFlyInTimer(gameState, &pos.x, &alpha, localIndex++, make_float2(0, 0), plane, gameModeState);
        
        float3 btnScale = make_float3(3*btnSize, 0.5f*btnSize, 1);
        pushRenderTexture(&gameState->renderer, make_transformX(pos, btnScale, make_float4(0, 0, 0, 1)), gameState->imageFiles.roundedRect, make_float4(1, 1, 1, alpha));
        renderText_centered(&gameState->renderer, &gameState->mainFont, "Play", pos.xy, BODY_FONT_SCALE, make_float4(0, 0, 0, 1));

        Rect2f bounds = make_rect2f_center_dim(pos.xy, btnScale.xy);
        if(in_rect2f_bounds(bounds, mouseWorldP) && custom_safeToProcessClick(gameState)) {
            if(gameState->mouseBtn[MOUSE_BUTTON_LEFT_CLICK] == MOUSE_BUTTON_PRESSED) {
                gameState->boardState = initBoardState(gameState->customBoardSize, &gameState->soundAssets, &gameState->settingsToSave, gameState->customMathSymbolsNonActive);
                goToGameMode(gameState, GAME_MODE_PLAY);
                resetUiFlyInTimersArray(gameState, true, gameModeState);
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
        pushRenderTexture(&gameState->renderer, make_transformX(pos, btnScale), gameState->imageFiles.back, make_float4(1, 1, 1, alpha));

        Rect2f bounds = make_rect2f_center_dim(pos.xy, scale_float2(2, btnScale.xy));
        if(in_rect2f_bounds(bounds, mouseWorldP) && !isGameTransitioningScene(gameState)) {
            if(gameState->mouseBtn[MOUSE_BUTTON_LEFT_CLICK] == MOUSE_BUTTON_PRESSED) {
                goToGameMode(gameState, GAME_START_SCREEN_MODE);
                resetUiFlyInTimersArray(gameState, true, gameModeState);
                resetUiFlyInTimersArray(gameState, false, GAME_START_SCREEN_MODE);
            }
        }
    }

    render_mergeRenderers(&gameState->renderer, tempRenderer);
}
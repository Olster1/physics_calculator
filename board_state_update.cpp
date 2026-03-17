inline bool numberChoiceMenuIdOpen(BoardState *state) {
    return state->numberChoiceMenuIdOpen >= 0;
}

inline bool numberChoiceMenuIsFinished(BoardState *state) {
    bool result = numberChoiceMenuIdOpen(state) && state->numberChoiceAnimationTimerOpen < 0;
    return result;
}

inline void closeNumberChoiceMenu(BoardState *state) {
    state->numberChoiceMenuIdOpen = -1;
}

inline void openNumberChoiceMenu(BoardState *state, int id, float2 pos) {
    state->numberChoiceAnimationTimerOpen = 0;
    state->numberChoiceMenuIdOpen = id;
    state->numberChoiceMenuPos = pos;
    state->numberChoiceActivateThisFrame = true;

}

float2 getNumberChoiceMenuPos(BoardState *state, float angle, float circleRadiusScale) {
    return plus_float2(scale_float2(0.5f*circleRadiusScale, make_float2(cos(angle), sin(angle))), state->numberChoiceMenuPos);
}

void respawnBoard(GameState *gameState) {
    assert(gameState->boardState.boardSize > 0);
    gameState->boardState = initBoardState(gameState->boardState.boardSize, &gameState->soundAssets, &gameState->settingsToSave, gameState->customMathSymbolsNonActive);
}

float2 cellPositionToWorldPosition(GameState *gameState, float2 cellPos, bool useOffset = false) {
    int cellSize = BOARD_CELL_SIZE;
    float2 centerOffset = make_float2(0.5f*gameState->boardState.boardSize*cellSize - 0.5f*cellSize, 0.5f*gameState->boardState.boardSize*cellSize - 0.5f*cellSize);

    float2 result = scale_float2(cellSize, cellPos); 
    result.x -= centerOffset.x;
    result.y -= centerOffset.y;

    if(useOffset) {
        result.x -= 0.3f*cellSize;
        result.y += 0.5f*cellSize;
    }
    return result;
}

void drawNumberChoiceMenu(GameState *gameState, Renderer *renderer, BoardState *state, float2 mouseWorldP, float2 plane) {
    if(numberChoiceMenuIdOpen(state)) {
        BoardValue *value = getBoardValueBySingleIndex(state, state->numberChoiceMenuIdOpen);

        float smallestDim = plane.x;
        if(plane.y < plane.x) {
            smallestDim = plane.y;
        }

        //NOTE: Number choice menu is open
        float cellSize = 0.15f*smallestDim;
        float angle = 0;
        int extraButtonCount = 0; //NOTE: Cancel
        bool hasEraser = false;
        if(value->value >= 0) {
            hasEraser = true;
            extraButtonCount++; //NOTE: Eraser
        }
        float angleOffset = TAU32 / (state->boardSize + extraButtonCount); 
        bool clicked = false;

        float circleMenuRadius = 0.6f*smallestDim;
        float alpha = 1.0f;

        if(state->numberChoiceAnimationTimerOpen >= 0) {
            state->numberChoiceAnimationTimerOpen += gameState->dt;

            float t = state->numberChoiceAnimationTimerOpen / MAX_NUMBER_CHOICE_ANIMATION;

            if(t > 1.0f) {
                t = 1.0f;
            }

            circleMenuRadius = lerp(0, circleMenuRadius, make_lerpTValue(Math3D_smoothstep01(t)));
            alpha = Math3D_smoothstep01(t);

            if(state->numberChoiceAnimationTimerOpen > MAX_NUMBER_CHOICE_ANIMATION) {
                state->numberChoiceAnimationTimerOpen = -1;
            }
        }

        //NOTE: Transparent overlay to see the menu better
        pushRenderTexture(&gameState->renderer, make_transformX(make_float3(0, 0, 0), make_float3(plane.x, plane.y, 1),  make_float4(0, 0, 0, 1)), gameState->imageFiles.whiteImage, make_float4(0, 0, 0, lerp(0, 0.4f, make_lerpTValue(alpha))));
        if(easyUi_isHoverAllowed(&gameState->uiState)) {
            //NOTE: Trigger the hover for the overlay so user has to click out first to click a button
        }

        pushRenderTexture(renderer, make_transformX(make_float3(state->numberChoiceMenuPos.x, state->numberChoiceMenuPos.y, MATH_3D_NEAR_CLIP_PlANE), make_float3(1.1f*circleMenuRadius, 1.1f*circleMenuRadius, 1), make_float4(0, 0, 0, 1)), gameState->imageFiles.numberBg);
        for(int i = 0; i < state->boardSize; ++i) {
            float2 pos = getNumberChoiceMenuPos(state, angle, circleMenuRadius);

            pushRenderTexture(renderer, make_transformX(make_float3(pos.x, pos.y, MATH_3D_NEAR_CLIP_PlANE), make_float3(1.4f*cellSize, 1.4f*cellSize, 1), make_float4(0, 0, 0, 1)), gameState->imageFiles.back2, make_float4(1, 1, 1, alpha));
            pushRenderTexture(renderer, make_transformX(make_float3(pos.x, pos.y, MATH_3D_NEAR_CLIP_PlANE), make_float3(cellSize, cellSize, 1), make_float4(0, 0, 0, 1)), gameState->imageFiles.circle, make_float4(1, 1, 1, alpha));
            
            renderText_centered(&gameState->renderer, &gameState->mainFont, easy_createString_printf(&globalPerFrameArena, "%d", i + 1), pos, BODY_FONT_SCALE, make_float4(0, 0, 0, alpha));
            angle += angleOffset;

            if(numberChoiceMenuIsFinished(state)) {
                Rect2f bounds = make_rect2f_center_dim(pos, make_float2(cellSize, cellSize));
                if(in_rect2f_bounds(bounds, mouseWorldP)) {
                    if(easyUi_isHoverAllowed(&gameState->uiState) && gameState->mouseBtn[MOUSE_BUTTON_LEFT_CLICK] == MOUSE_BUTTON_PRESSED) {
                        clicked = true;
                            
                        int newValue = i + 1;
                        setBoardValueCell(state, state->numberChoiceMenuIdOpen, value, newValue, &gameState->soundAssets, &gameState->settingsToSave);
                        
                        closeNumberChoiceMenu(state);
                    }
                }
            }
        }

        {
            float2 pos = getNumberChoiceMenuPos(state, angle, circleMenuRadius);

            if(hasEraser) {
                pos = getNumberChoiceMenuPos(state, angle, circleMenuRadius);
                pushRenderTexture(renderer, make_transformX(make_float3(pos.x, pos.y, MATH_3D_NEAR_CLIP_PlANE), make_float3(cellSize, cellSize, 1), make_float4(0, 0, 0, 1)), gameState->imageFiles.eraseSelection, make_float4(1, 1, 1, alpha));

                Rect2f bounds = make_rect2f_center_dim(pos, make_float2(cellSize, cellSize));
                if(numberChoiceMenuIsFinished(state) && in_rect2f_bounds(bounds, mouseWorldP)) {
                    if(easyUi_isHoverAllowed(&gameState->uiState) && gameState->mouseBtn[MOUSE_BUTTON_LEFT_CLICK] == MOUSE_BUTTON_PRESSED) {
                        setBoardValueCell(state, state->numberChoiceMenuIdOpen, value, -1, &gameState->soundAssets, &gameState->settingsToSave);
                        
                    }
                }
            }
        }

        if(numberChoiceMenuIsFinished(state) && !state->numberChoiceActivateThisFrame && !clicked && (gameState->mouseBtn[MOUSE_BUTTON_LEFT_CLICK] == MOUSE_BUTTON_PRESSED)) {
            closeNumberChoiceMenu(state);
            state->numberChoiceMenuPos = make_float2(0, 0);
        }
    }
    state->numberChoiceActivateThisFrame = false;
}

bool drawImageWithInteraction(GameState *gameState, float2 mouseWorldP, Texture *texture, float3 pos, float2 scale, char *label, bool withBacking = false) {
    bool result = false;

    if(withBacking) {
        pushRenderTexture(&gameState->renderer, make_transformX(pos, make_float3(2*scale.x, 2*scale.y, 1), make_float4(0, 0, 0, 1)), gameState->imageFiles.back2, make_float4(1, 1, 1, 1));
    }

    pushRenderTexture(&gameState->renderer, make_transformX(pos, make_float3(scale.x, scale.y, 1), make_float4(0, 0, 0, 1)), texture, (withBacking) ? make_float4(1, 1, 1, 1) : make_float4(1, 1, 1, 1));
    Rect2f bounds = make_rect2f_center_dim(pos.xy, make_float2((withBacking) ? 2*scale.x : scale.x, (withBacking) ? 2*scale.y : scale.y));
    if(in_rect2f_bounds(bounds, mouseWorldP)) {
        if(easyUi_isHoverAllowed(&gameState->uiState) && gameState->mouseBtn[MOUSE_BUTTON_LEFT_CLICK] == MOUSE_BUTTON_PRESSED) {
            result = true;
        }
    }
    if(label) {
        renderText_centered(&gameState->renderer, &gameState->mainFont, label, make_float2(pos.x, pos.y - 0.6f*scale.y), 0.5f*BODY_FONT_SCALE, make_float4(0, 0, 0, 1));
    }
    return result;
}

void processUndoAction(BoardState *state, SoundAssets *soundAssets) {
    UndoRedoBlock *block = state->undoRedoBlock;
    assert(block);
    if(block && !block->isSentinel) {
        BoardValue *value = getBoardValueBySingleIndex(state, block->cellIndex);
        value->value = block->prevValue;
        value->alertTimer = 0;
        if(value->value >= 1) {
            playSoundForeground(&soundAssets->placeSound);
        } else {
            playSoundForeground(&soundAssets->eraseSound);
        }

        state->undoRedoBlock = block->next;
    }
}

void processRedoAction(BoardState *state, SoundAssets *soundAssets) {
    UndoRedoBlock *block = state->undoRedoBlock;
    if(block && block->prev) {
        block = block->prev;
        BoardValue *value = getBoardValueBySingleIndex(state, block->cellIndex);
        value->value = block->value;
        value->alertTimer = 0;
        if(value->value >= 1) {
            playSoundForeground(&soundAssets->placeSound);
        } else {
            playSoundForeground(&soundAssets->eraseSound);
        }

        state->undoRedoBlock = block;
    }
}

void getHint(BoardState *state, GameState *gameState) {
    int indexCount = 0;
    int *indexes = pushArray(&globalPerFrameArena, state->boardSize*state->boardSize, int);

    for(int y = 0; y < state->boardSize; ++y) {
        for(int x = 0; x < state->boardSize; ++x) {
            BoardValue *value = getBoardValue(state, x, y);
            if(value->value != value->targetValue) {
                //NOTE: Value not set so is valid to show
                assert(indexCount < (state->boardSize*state->boardSize));
                indexes[indexCount++] = y*state->boardSize + x;
            }
        }
    }

    if(indexCount > 0) {
        int hintIndex = indexes[random_between_int(0, indexCount)];
        BoardValue *value = getBoardValueBySingleIndex(state, hintIndex);
        setBoardValueCell(state, hintIndex, value, value->targetValue, &gameState->soundAssets, &gameState->settingsToSave);
    }
}

bool checkBoardForSolve(BoardState *state) {
    bool solved = true;

    //NOTE: Now check the math groups
    for(int i = 0; i < state->mathGroups.typeCount && solved; i++) {
        MathGroupPartner *partner = state->mathGroups.mathTypes + i;

        // BoardMathType type;
        int answer = partner->answer;

        int total = 0;
        for(int j = 0; j < partner->size; j++) {
            float2 p = partner->positions[j];
            BoardValue *value = getBoardValue(state, p.x, p.y);

            total = getMathGroupTotal(total, partner->type, value->value, j);
        }

        if(total != answer) {
            for(int j = 0; j < partner->size; j++) {
                float2 p = partner->positions[j];
                BoardValue *value = getBoardValue(state, p.x, p.y);
                value->alertTimer = 0;
            }
            solved = false;
        }
    }
    
    if(solved) {
        for(int y = 0; y < state->boardSize && solved; ++y) {
            for(int x = 0; x < state->boardSize && solved; ++x) {
                BoardValue *value = getBoardValue(state, x, y);
                if(value->value >= 0) {
                    for(int j = 0; j < state->boardSize && solved; ++j) {
                        if(j != x) {
                            BoardValue *valueTest = getBoardValue(state, j, y);

                            if(valueTest->value == value->value) {
                                //NOTE: Not unique so not solved
                                solved = false;
                                value->alertTimer = 0;
                            }
                        }
                    }

                    for(int j = 0; j < state->boardSize && solved; ++j) {
                        if(j != y) {
                            BoardValue *valueTest = getBoardValue(state, x, j);

                            if(valueTest->value == value->value) {
                                //NOTE: Not unique so not solved
                                solved = false;
                                value->alertTimer = 0;
                            }
                        }
                    }
                } else {
                    value->alertTimer = 0;
                    solved = false;
                }
            }
        }

    
        
    }

    return solved;
}

void resetUiFlyInTimersArray(GameState *gameState, bool isReverse, GameModeState gameMode) {
    int min = 0;
    int max = MAX_UI_FLY_IN_TIMERS;
    getUiFlyInRange(gameMode, &min, &max);
    for(int i = min; i < max; ++i) {
        gameState->uiFlyInTimers[i].value = 0;
        gameState->uiFlyInTimers[i].reverse = isReverse;
    }
}

float getBackButtonOffset(float2 plane) {
    float biggerDim = plane.x;
    if(plane.y > plane.x) {
        biggerDim = plane.y;
    }

    float btnSize = 0.1f*biggerDim;
    return 0.5f*btnSize;
}

float getBackButtonSize(float2 plane) {
    float dim = plane.x;
    if(plane.y > plane.x) {
        dim = plane.y;
    }

    float btnSize = 0.04f*dim;
    return btnSize;
}

void drawUi(GameState *gameState, Renderer *renderer, BoardState *state, float2 mouseWorldP, float2 planeSize) {
    int buttonCount = 5;
    
    float dim = planeSize.x;
    if(planeSize.y < planeSize.x) {
        dim = planeSize.y;
    }

    float buttonSize = dim / (buttonCount + 2);
    float xAt = -0.5f*buttonCount*buttonSize + 0.5f*buttonSize;
    float yAt = -0.5f*planeSize.y + 0.5f*buttonSize + 2;
    
    float2 increment = make_float2(buttonSize, 0);

    if(planeSize.y < planeSize.x) {
        increment.x = 0;
        increment.y = buttonSize;
        xAt = 0.5f*planeSize.x - 0.5f*buttonSize;
        yAt = -0.5f*buttonCount*buttonSize + 0.5f*buttonSize;
    }
    
    float buttonScale = 0.1f*dim;
    float2 at = make_float2(xAt, yAt);

    {
        float backBtnOffset = getBackButtonOffset(planeSize);
        float backBtnSize = getBackButtonSize(planeSize);
    
        if(drawImageWithInteraction(gameState, mouseWorldP, gameState->imageFiles.back, make_float3(-0.5f*planeSize.x + backBtnOffset, 0.5f*planeSize.y - backBtnOffset, 0), make_float2(backBtnSize, backBtnSize), 0, true)) {
            gameState->gameModeState = GAME_START_SCREEN_MODE;
            resetUiFlyInTimersArray(gameState, false, GAME_START_SCREEN_MODE);
            saveSettingsFile(&gameState->settingsToSave, 0);
        }
    }

    if(planeSize.y < planeSize.x) {
        pushRenderTexture(renderer, make_transformX(make_float3(0.5f*planeSize.x - 0.5f*buttonSize, 0, 1), make_float3(buttonSize, (buttonCount + 2)*buttonSize, 1)), gameState->imageFiles.back1, make_float4(1, 1, 1, 1));    
    } else {
        pushRenderTexture(renderer, make_transformX(make_float3(0, yAt - 1, 1), make_float3((buttonCount + 2)*buttonSize, buttonSize, 1)), gameState->imageFiles.back3, make_float4(1, 1, 1, 1));    
    }

    if(drawImageWithInteraction(gameState, mouseWorldP, gameState->imageFiles.hint, make_float3(at.x, at.y, 0), make_float2(buttonScale, buttonScale), "Hint")) {
        getHint(state, gameState);
    }
    at = plus_float2(at, increment);

    if(drawImageWithInteraction(gameState, mouseWorldP, gameState->imageFiles.undo, make_float3(at.x, at.y, 0), make_float2(buttonScale, buttonScale), "Undo")) {
        processUndoAction(&gameState->boardState, &gameState->soundAssets);
    }
    at = plus_float2(at, increment);

    if(drawImageWithInteraction(gameState, mouseWorldP, gameState->imageFiles.redo, make_float3(at.x, at.y, 0), make_float2(buttonScale, buttonScale), "Redo")) {
        processRedoAction(&gameState->boardState, &gameState->soundAssets);
    }
    at = plus_float2(at, increment);

    if(drawImageWithInteraction(gameState, mouseWorldP, gameState->imageFiles.refresh, make_float3(at.x, at.y, 0), make_float2(buttonScale, buttonScale), "Refresh")) {
        respawnBoard(gameState);
    }
    at = plus_float2(at, increment);
    if(drawImageWithInteraction(gameState, mouseWorldP, gameState->imageFiles.light, make_float3(at.x, at.y, 0), make_float2(buttonScale, buttonScale), "Check")) 
    {
        bool solved = checkBoardForSolve(state);

        printf("solved: %d\n", solved);
    }
    at = plus_float2(at, increment);

}


int getGroupValueFromBoard(BoardState *state, int x, int y) {
    int *boardGroups = state->mathGroups.mathGroupIds;
    if(x >= 0 && x < state->boardSize && y >= 0 && y < state->boardSize) {
        return boardGroups[y*state->boardSize + x];
    } else {
        return -1;
    }
}


Texture *getBackDropTextureForMathGroup(GameState *gameState, BoardState *state, int x, int y) {
    return gameState->imageFiles.roundedRect;
    Texture *result = gameState->imageFiles.textBackdrop;

    float2 offsets[] = {make_float2(1, 0), make_float2(-1, 0), make_float2(0, -1), make_float2(0, 1)};
    u32 boardState = 0;

    int thisGroupValue = getGroupValueFromBoard(state, x, y);
    
    for(int i = 0; i < arrayCount(offsets); i++) {
        if(getGroupValueFromBoard(state, x + offsets[i].x, y + offsets[i].y) == thisGroupValue) {
            boardState |= (1 << i);
        }
    }

    switch(boardState) {
        case 0: { //Nothing
            result = gameState->imageFiles.outlineSingle;
        } break;
         case 1: { // Right
            result = gameState->imageFiles.outlineLeftEnd;
        } break;
         case 2: { // Left
            result = gameState->imageFiles.outlineRightEnd;
        } break;
         case 3: { // Right & Left
            result = gameState->imageFiles.outlineTwoAcross;
        } break;
         case 4: {  //Down
            result = gameState->imageFiles.outlineTopEnd;
        } break;
         case 5: {  // Right & Down
            result = gameState->imageFiles.outlineCornerTopLeft;
        } break;
         case 6: { // Left & Down 
            result = gameState->imageFiles.outlineCornerTopRight;
        } break;
         case 7: { // Left, Right & Down
            result = gameState->imageFiles.bottomLeftRight;
        } break;
         case 8: { //Top
            result = gameState->imageFiles.outlineBottomEnd;
        } break;
         case 9: { //Right & Up
            result = gameState->imageFiles.outlineCornerBottomLeft;
        } break;
         case 10: { //Left & Up
            result = gameState->imageFiles.outlineCornerBottomRight;
        } break;
        case 11: {  //Left, Right & Up
            result = gameState->imageFiles.topLeftRight;
        } break;
        case 12: { //Up & Down
            result = gameState->imageFiles.outlineTwoUp;
        } break;
        case 13: { //Up & Down & Right
            result = gameState->imageFiles.topBottomRight;
        } break;
        case 14: {//Up & Down & Left
            result = gameState->imageFiles.topBottomLeft;
        } break;
        case 15: { //NOTE: All 
            result = gameState->imageFiles.outlineAllDirections;
        } break;
    } 
  
    return result;
}

void game_updateCameraDragging(GameState *gameState, float2 mouseWorldP) {
    if(easyUi_isHoverAllowed(&gameState->uiState) && gameState->mouseBtn[MOUSE_BUTTON_RIGHT_CLICK] == MOUSE_BUTTON_PRESSED) {
        gameState->isDraggingCamera = true;
        gameState->cameraDragStart = mouseWorldP;
    }

    if(gameState->mouseBtn[MOUSE_BUTTON_RIGHT_CLICK] == MOUSE_BUTTON_RELEASED || gameState->mouseBtn[MOUSE_BUTTON_RIGHT_CLICK] == MOUSE_BUTTON_NONE) {
        gameState->isDraggingCamera = false;
        gameState->camera.targetP = gameState->camera.T.pos.xy;
    }

    if(gameState->isDraggingCamera) {
        gameState->camera.targetP = plus_float2(gameState->camera.T.pos.xy, minus_float2(mouseWorldP, gameState->cameraDragStart));
    }
}

void drawBoard(GameState *gameState, Renderer *renderer, BoardState *state, float2 mouseWorldP, float2 planeSize) {
    int cellSize = BOARD_CELL_SIZE;
    float bgMargin = 40 ;
    float2 centerOffset = make_float2(0.5f*state->boardSize*cellSize - 0.5f*cellSize, 0.5f*state->boardSize*cellSize - 0.5f*cellSize);
    
    Renderer *tempRenderer = pushStruct(&globalPerFrameArena, Renderer);

    // pushRenderTexture(&gameState->renderer, make_transformX(make_float3(0, 0, 1), make_float3(cellSize*state->boardSize + bgMargin, cellSize*state->boardSize + bgMargin, 1), make_float4(0, 0, 0, 1)), gameState->imageFiles.back4, make_float4(1, 1, 1, 1));

    //NOTE: Update dragging the camera
    game_updateCameraDragging(gameState, mouseWorldP);

    int id = 0;
    for(int y = 0; y < state->boardSize; ++y) {
        for(int x = 0; x < state->boardSize; ++x) {

            float2 cellScale = make_float2(1, 1);
            
            float yPos = y*cellSize - centerOffset.y;
            BoardValue *value = getBoardValue(state, x, y);
            if(value->animationFlyInTimer >= 0) {
                value->animationFlyInTimer += gameState->dt;
                float t = value->animationFlyInTimer / value->maxAnimationFlyInTimer;
                t = Math3D_smoothstep01(t);
                if(value->animationFlyInTimer > value->maxAnimationFlyInTimer) {
                    value->animationFlyInTimer = -1;
                    t = 1;
                }

                yPos = lerp(value->animationStartPosY, yPos,  make_lerpTValue(t));
            }

            Renderer *rendererToAddTo = &gameState->renderer;
            if(value->alertTimer >= 0) {
                value->alertTimer += gameState->dt;

                float p = value->alertTimer / MAX_CELL_ALERT_TIME_ANIMATION;

                cellScale.x = 1.0f + 0.5f * sinf(PI32 * p);
                cellScale.y = cellScale.x;

                if(value->alertTimer >= MAX_CELL_ALERT_TIME_ANIMATION) {
                    value->alertTimer = -1;
                }
                rendererToAddTo = tempRenderer;
            }

            float3 pos = make_float3(x*cellSize - centerOffset.x, yPos, MATH_3D_NEAR_CLIP_PlANE);

            float4 color = global_board_colors[((5*value->mathGroupId) + state->colorOffset) % arrayCount(global_board_colors)];
            
            pushRenderTexture(rendererToAddTo, make_transformX(pos, make_float3(cellScale.x*cellSize, cellScale.y*cellSize, 1), make_float4(0, 0, 0, 1)), getBackDropTextureForMathGroup(gameState, state, x, y), color);
            if(value->value >= 0) {
                renderText_centered(rendererToAddTo, &gameState->mainFont, easy_createString_printf(&globalPerFrameArena, "%d", value->value), pos.xy, BODY_FONT_SCALE, make_float4(0, 0, 0, 1));
            }

            //NOTE: Render the target value
            // renderText_centered(&gameState->renderer, &gameState->mainFont, easy_createString_printf(&globalPerFrameArena, "%d", value->targetValue), pos.xy, BODY_FONT_SCALE, make_float4(1, 0.5f, 0, 1));

            Rect2f bounds = make_rect2f_center_dim(pos.xy, make_float2(cellSize, cellSize));
            if(in_rect2f_bounds(bounds, mouseWorldP)) {
                if(easyUi_isHoverAllowed(&gameState->uiState) && gameState->mouseBtn[MOUSE_BUTTON_LEFT_CLICK] == MOUSE_BUTTON_PRESSED) {
                    if(!numberChoiceMenuIdOpen(state)) {
                        openNumberChoiceMenu(state, id, make_float2(0, 0)); //pos.xy
                    }
                }
            }
            id++;
        }
    }   

    float alpha = 1;
    if(state->mathGroupFadeInTimer >= 0) {
        state->mathGroupFadeInTimer += gameState->dt;
        
        alpha = state->mathGroupFadeInTimer / MAX_MATH_GROUP_FADE_IN_TIMER;
        alpha = Math3D_smoothstep01(alpha);

        if(state->mathGroupFadeInTimer >= MAX_MATH_GROUP_FADE_IN_TIMER) {
            alpha = 1.0f;
            state->mathGroupFadeInTimer = -1;
        }
    }
    
    for(int i = 0; i < state->mathGroups.typeCount; i++) {
        MathGroupPartner *partner = state->mathGroups.mathTypes + i;
        // BoardMathType type;
        char *symbol = "";
        Texture *symbolTexture = 0;

        if(partner->type == BOARD_MATH_TYPE_ADD) {
            symbol = "+";
            symbolTexture = gameState->imageFiles.add;
        }
        if(partner->type == BOARD_MATH_TYPE_MULTIPLY) {
            symbol = "x";
            symbolTexture = gameState->imageFiles.multiply;
        }
        if(partner->type == BOARD_MATH_TYPE_SUBTRACT) {
            symbol = "-";
            symbolTexture = gameState->imageFiles.minus;
        }
        if(partner->type == BOARD_MATH_TYPE_DIVIDE) {
            symbol = "/";
            symbolTexture = gameState->imageFiles.divide;
        }

        float2 totalPos = cellPositionToWorldPosition(gameState, partner->farthestPosition, true);
        float fontScale = 0.8f*BODY_FONT_SCALE;
        char *mathString = easy_createString_printf(&globalPerFrameArena, "%d", partner->answer);
        float2 backdropDim = renderText_getDim(&gameState->renderer, &gameState->mainFont, mathString, fontScale);
        float2 bounds = backdropDim;

        float minSize = 5;
        if(backdropDim.x < minSize) {
            backdropDim.x = minSize;
        }
        if(backdropDim.y < minSize) {
            backdropDim.y = minSize;
        }   

        backdropDim.x += 4;
        backdropDim.y += 1;

        float symbolScale = 2;
        if(!symbolTexture) {
            symbolScale = 0;    
        }

        pushRenderTexture(renderer, make_transformX(make_float3(totalPos.x, totalPos.y, 1), make_float3(backdropDim.x, backdropDim.y, 1), make_float4(0, 0, 0, 1)), gameState->imageFiles.circle, make_float4(1, 1, 1, alpha));
        renderText_centered(&gameState->renderer, &gameState->mainFont, mathString, make_float2(totalPos.x - symbolScale, totalPos.y), fontScale, make_float4(0, 0, 0, alpha));
        if(symbolTexture) {
            float margin = 0.5f*bounds.x;
            if(margin < 1) {
                margin = 1;
            }
            float3 symbolPos = make_float3(totalPos.x + margin, totalPos.y, 1);
            float3 symbolScale1 = make_float3(symbolScale, symbolScale, 1);
            pushRenderTexture(renderer, make_transformX(symbolPos, scale_float3(2.0f, symbolScale1), make_float4(0, 0, 0, 1)), gameState->imageFiles.back2, make_float4(1, 1, 1, alpha));
            pushRenderTexture(renderer, make_transformX(symbolPos, scale_float3(1.8f, symbolScale1), make_float4(0, 0, 0, 1)), gameState->imageFiles.circle, make_float4(1, 1, 1, alpha));
            pushRenderTexture(renderer, make_transformX(symbolPos, symbolScale1, make_float4(0, 0, 0, 1)), symbolTexture, make_float4(0, 0, 0, alpha));
        } 
    }

    render_mergeRenderers(&gameState->renderer, tempRenderer);
}

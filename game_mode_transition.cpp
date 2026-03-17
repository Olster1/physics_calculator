void goToGameMode(GameState *gameState, GameModeState mode) {
    if(gameState->targetGameMode != mode) {
        gameState->gameModeFadeTimer = MAX_FADE_TIME;
        gameState->gameModeFadeDirection = -1; //NOTE: Fade Out
        gameState->targetGameMode = mode;
    }
}

void gameTransitionCallback(GameState *gameState) {
	
	
}

bool isGameTransitioningScene(GameState *gameState) {
	return gameState->gameModeFadeTimer >= 0;
}

void updateFadeScreen(GameState *gameState, float dt, float w, float h) {
    
	if(gameState->gameModeFadeTimer >= 0) {
        
		gameState->gameModeFadeTimer -= dt;

		float useValue = gameState->gameModeFadeTimer;
		int direction = gameState->gameModeFadeDirection;

		if(gameState->gameModeFadeTimer <= 0) {
			if(gameState->targetGameMode != GAME_MODE_STATE_NONE) {
				gameState->gameModeState = gameState->targetGameMode;
				gameState->gameModeFadeTimer = MAX_FADE_TIME;
				gameState->gameModeFadeDirection *= -1;
				gameState->targetGameMode = GAME_MODE_STATE_NONE;

				//NOTE: Do stuff inbetween
				gameTransitionCallback(gameState);
				

			} else {
				gameState->gameModeFadeTimer = -1;
			}
		} 
		
		float transparent = clamp(0, 1, (useValue / MAX_FADE_TIME));
		
		if(direction < 0) {
			transparent = 1.0f - transparent;
		}

        // pushRenderTexture(&gameState->renderer, make_transformX(make_float3(0, 0, 0), make_float3(w, h, 1),  make_float4(0, 0, 0, 1)), gameState->imageFiles.whiteImage, make_float4(0, 0, 0, transparent));
	}
}

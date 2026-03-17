#include "./includes.h"

void updateGame(GameState *gameState) {
    assert(gameState->initialized);

    float scaleFactor = 0.1f;
    float2 plane = make_float2(scaleFactor*gameState->settingsToSave.windowX, scaleFactor*gameState->settingsToSave.windowY);
    float16 viewT = make_ortho_matrix_origin_center(plane.x, plane.y, MATH_3D_NEAR_CLIP_PlANE, MATH_3D_FAR_CLIP_PlANE);
    float16 worldToCameraT = makeWorldToCameraT(&gameState->camera);
    float2 mouseWorldP = getWorldPFromMouse(gameState, plane, true);

    pushRenderView(&gameState->renderer, viewT); 

    if(gameState->enterPressed && gameState->stringBuffer.string) {
        refreshVmMemoryArena();
        clearResizeArray(gameState->operations);
        gameState->calculatorLineCount = 0;

        //TODO: Memory leak if we clear the whole cacluator, use another lifetime arena
        gameState->codeToRun = easy_createString_printf(&globalLongTermArena, "%s;%s",  gameState->codeToRun, gameState->stringBuffer.string);

        //TODO: Run through all code to find number of new lines
        int numberOfLines = 0; //ERROR: codeToRun;

        gameState->calculatorLines = pushArray(&globalPerVmRunLifetime, numberOfLines, CalculatorLine);
        gameState->stringBuffer.string = 0;

        compileToByteCode(gameState->codeToRun, &gameState->operations);
       
        runCode(gameState, gameState->operations, getArrayLength(gameState->operations));
    }

    float lineHeight = 5;
    float2 maxBufferSize = make_float2(0, 0);
    float2 at = make_float2(-0.5f*plane.x - gameState->bufferOffset.x, -0.5f*plane.y + 5 - gameState->bufferOffset.y);
    for(int i = gameState->calculatorLineCount - 1; i >= 0; --i) {
        CalculatorLine *b = gameState->calculatorLines + i;
        assert(b->in);
        Rect2f dim = renderText(&gameState->renderer, &gameState->mainFont, b->in, at, BODY_FONT_SCALE, make_float4(0, 0, 0, 1));
        at.y += lineHeight;
        maxBufferSize.y += lineHeight;

        if(get_scale_rect2f(dim).x > maxBufferSize.x) {
            maxBufferSize.x = get_scale_rect2f(dim).x;
        }
        
    }

    if(gameState->stringBuffer.string) {
        Rect2f dim = renderText(&gameState->renderer, &gameState->mainFont, gameState->stringBuffer.string, make_float2(-0.5f*plane.x - gameState->bufferOffset.x, -0.5f*plane.y + 1 -  gameState->bufferOffset.y), BODY_FONT_SCALE, make_float4(0, 0, 0, 1));

        if(get_scale_rect2f(dim).x > maxBufferSize.x) {
            maxBufferSize.x = get_scale_rect2f(dim).x;
        }
    }

    gameState->bufferOffset = plus_float2(gameState->bufferOffset, gameState->scrollWheelDelta);
    gameState->bufferOffset.x = clamp(0, MathMaxf(0, maxBufferSize.x - plane.x), gameState->bufferOffset.x);
    gameState->bufferOffset.y = clamp(0, MathMaxf(0, maxBufferSize.y - plane.y), gameState->bufferOffset.y);


    easyUi_clearHoverId(&gameState->uiState);
}
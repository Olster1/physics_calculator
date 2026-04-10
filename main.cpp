#include "./includes.h"

Rect2f renderTextAsTokens(GameState *gameState, char *string, float2 at, float2 plane, float scaleFactor = 1.0f, int cursorLocation = -1) {
    Rect2f totalBounds = make_rect2f_inverse_infinity();
    EasyTokenizer tokenizer = lexBeginParsing(string, EASY_LEX_OPTION_NONE);

    float2 startP = at;

    Rect2f cursorRect = {};
    bool parsing = true;
    int index = 0;
    while(parsing) {

        EasyToken token = lexGetNextToken(&tokenizer);
        if(token.type == TOKEN_NULL_TERMINATOR) {
            parsing = false;
        } else {

            float4 text_color = gameState->colorPallette->standard;
            if(token.type == TOKEN_INTEGER || token.type == TOKEN_FLOAT) {
                text_color = gameState->colorPallette->variable;
            } else if(token.type == TOKEN_STRING) {
                text_color = gameState->colorPallette->string;
            } else if(token.type == TOKEN_OPEN_BRACKET || token.type == TOKEN_CLOSE_BRACKET || token.type == TOKEN_OPEN_SQUARE_BRACKET || token.type == TOKEN_CLOSE_SQUARE_BRACKET) {
                text_color = gameState->colorPallette->bracket;
            } else if(token.type == TOKEN_WORD) {
                text_color = gameState->colorPallette->keyword;
            } else if(token.isKeyword || token.isType) {
                text_color = gameState->colorPallette->keyword;
            } else if(token.type == TOKEN_COMMENT) {
                text_color = gameState->colorPallette->comment;
            } else if(token.type == TOKEN_PREPROCESSOR) {
                text_color = gameState->colorPallette->preprocessor;
            }

            char *strToDraw = nullTerminateArena(token.at, token.size, &globalPerFrameArena);
            Rect2f bounds = renderText(&gameState->renderer, &gameState->mainFont, strToDraw, at, scaleFactor*BODY_FONT_SCALE, text_color, true, cursorLocation - index, &cursorRect);
            at.x += get_scale_rect2f(bounds).x;

            totalBounds = rect2f_union(totalBounds, bounds);

            index += token.size;

        }
    }
    //NOTE: Draw the cursor location
    if(cursorLocation >= 0) {
        float lineHeight = 5;
        if(cursorLocation == 0) {
            cursorRect.maxX = startP.x;
        }
        pushRenderTexture(&gameState->renderer, make_transformX_float2(make_float2(cursorRect.maxX, -0.5f*plane.y + 0.5f*lineHeight), make_float2(0.1, lineHeight)), gameState->imageFiles.whiteImage, MY_COLOR_PASTEL_LAVENDER);
    }
    return totalBounds;
}

void runCalculator(GameState *gameState) {
    //TODO: Memory leak if we clear the whole calculator, use another lifetime arena
    char *codeToRun = easy_createString_printf(&globalPerClearSessionArena, "%s%s;",  gameState->codeToRun, gameState->stringBuffer.string);

    int numberOfLines = 0;
    {
        //NODE: Run through all code to find number of new lines
        char *str = codeToRun;
        while(*str) {
            if(*str == ';') {
                numberOfLines++;
            }
            str++;
        }

        //NOTE: Allocate the array
        gameState->maxCalculatorLineCount = numberOfLines;
        gameState->calculatorLines = pushArray(&globalPerVmRunLifetime, numberOfLines, CalculatorLine);

        //NOTE: Loop through again and set the strings
        char *start = codeToRun;
        str = codeToRun;
        int lineAt = 0;
        while(*str) {
            if(*str == ';') {
                gameState->calculatorLines[lineAt++].in = nullTerminateArena(start, (int)(str - start), &globalPerVmRunLifetime);
                start = str + 1;
            }
            str++;
        }
    }


    bool error = compileToByteCode(codeToRun, &gameState->operations);

    bool clear = false;
    if(!error) {
        VmMachineState machineState  = initVmMachineState(gameState->startUseRadians);
        clear = runCode(&machineState, gameState, gameState->operations);
    }

    if(!error) {
        //NOTE: Keep the buffer the user wrote if there was an error parsing it
        clearStringBuffer(&gameState->stringBuffer);
    }

    if(!error && !clear) {
        gameState->codeToRun = codeToRun;
    }
}


void updateGame(GameState *gameState) {
    assert(gameState->initialized);

    backend_render_clearFrame(gameState->colorPallette->background);

    float scaleFactor = 0.1f;
    float2 plane = make_float2(scaleFactor*gameState->settingsToSave.windowX, scaleFactor*gameState->settingsToSave.windowY);
    float16 viewT = make_ortho_matrix_origin_center(plane.x, plane.y, MATH_3D_NEAR_CLIP_PlANE, MATH_3D_FAR_CLIP_PlANE);
    float16 worldToCameraT = makeWorldToCameraT(&gameState->camera);
    float2 mouseWorldP = getWorldPFromMouse(gameState, plane, true);

    pushRenderView(&gameState->renderer, viewT);


    if(gameState->enterPressed && easyString_getStringLength_utf8(gameState->stringBuffer.string) > 0) {
        //NOTE: We handle clear the buffer here instead of trying to parse the command since it's not a function it's just one word
        if(easyString_stringsMatch_nullTerminated(gameState->stringBuffer.string, "clear")) {
            //NOTE: Clear the buffer
            clearCalculatorBuffer(gameState);
            clearStringBuffer(&gameState->stringBuffer);
        } else {
            refreshVmMemoryArena();
            gameState->operations.clear();
            gameState->calculatorLineCount = 0;

            //NOTE: Add string to history
            gameState->bufferHistory.push(easy_createString_printf(&globalLongTermArena, "%s", gameState->stringBuffer.string));
            gameState->historyAt = gameState->bufferHistory.count;
            runCalculator(gameState);
        }

    }
    float lineHeight = 5;
    float marginSpace = 5;
    float2 maxBufferSize = make_float2(0, 0);

    //NOTE: Draw the input buffer
    pushRenderTexture(&gameState->renderer, make_transformX_float2(make_float2(0, -0.5f*plane.y + 0.5f*lineHeight), make_float2(plane.x, lineHeight)), gameState->imageFiles.whiteImage, gameState->colorPallette->backgroundVariation);
    if(gameState->stringBuffer.string) {
        float sideOffset = 1;
        float2 at = make_float2(-0.5f*plane.x - gameState->bufferOffset.x + sideOffset, -0.5f*plane.y + 1 -  gameState->bufferOffset.y);

        Rect2f dim = renderTextAsTokens(gameState, gameState->stringBuffer.string, at, plane, 1.0f, gameState->stringBuffer.cursor);


        if(get_scale_rect2f(dim).x > maxBufferSize.x) {
            maxBufferSize.x = get_scale_rect2f(dim).x;
        }
    }


    //NOTE: Draw radians or degrees mode
    {
        float scale = 0.8f*BODY_FONT_SCALE;
        float2 at = make_float2(0.5*plane.x, 0.5*plane.y - scale*gameState->mainFont.fontHeight);
        char *angleMode = "rad";
        if(!gameState->useRadians) {
            angleMode = "deg";
        }
        Rect2f bounds = renderText(&gameState->renderer, &gameState->mainFont, angleMode, at, scale, gameState->colorPallette->standard, false);
        at.x -= get_scale_rect2f(bounds).x + 2;
        renderText(&gameState->renderer, &gameState->mainFont, angleMode, at, scale, gameState->colorPallette->standard);
    }


    float startX = -0.5f*plane.x - gameState->bufferOffset.x;
    float2 at = make_float2(startX, -0.5f*plane.y + 5 - gameState->bufferOffset.y);
    for(int i = gameState->calculatorLineCount - 1; i >= 0; --i) {
        at.x = startX;
        CalculatorLine *b = gameState->calculatorLines + i;
        assert(b->in);
        assert(b->out);

        Rect2f dim = renderTextAsTokens(gameState, b->out, at, plane);

        // pushRenderTexture(&gameState->renderer, make_transformX_float2(get_centre_rect2f(dim), get_scale_rect2f(dim)), gameState->imageFiles.whiteImage, MY_COLOR_PASTEL_LAVENDER);

        at.x += get_scale_rect2f(dim).x + marginSpace;
        Rect2f dim1 = renderTextAsTokens(gameState, b->in, at, plane, 0.8f);

        dim = rect2f_union(dim, dim1);

        at.y += lineHeight;
        maxBufferSize.y += lineHeight;

        if(get_scale_rect2f(dim).x > maxBufferSize.x) {
            maxBufferSize.x = get_scale_rect2f(dim).x;
        }
    }



    gameState->bufferOffset = plus_float2(gameState->bufferOffset, gameState->scrollWheelDelta);
    gameState->bufferOffset.x = clamp(0, MathMaxf(0, maxBufferSize.x - plane.x), gameState->bufferOffset.x);
    gameState->bufferOffset.y = clamp(0, MathMaxf(0, maxBufferSize.y - plane.y), gameState->bufferOffset.y);


    easyUi_clearHoverId(&gameState->uiState);
}
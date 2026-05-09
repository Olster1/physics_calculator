#include "./includes.h"

Rect2f renderTextAsTokens(GameState *gameState, char *string, float2 at, float2 plane, float scaleFactor = 1.0f, int cursorLocation = -1) {
    float2 cursorP = {};
    Rect2f totalBounds = renderText(&gameState->renderer, &gameState->mainFont, string, at, scaleFactor*BODY_FONT_SCALE, MY_COLOR_WHITE, true, cursorLocation, &cursorP, gameState->colorPallette);

    //NOTE: Draw the cursor location
    if(cursorLocation >= 0) {
        float lineHeight = 5;
        if(cursorLocation == 0) {
            cursorP.x = at.x;
        }
        pushRenderTexture(&gameState->renderer, make_transformX_float2(make_float2(cursorP.x, -0.5f*plane.y + 0.5f*lineHeight), make_float2(0.1, lineHeight)), gameState->imageFiles.whiteImage, gameState->colorPallette->preprocessor);
    }
    return totalBounds;
}

void updateGame(GameState *gameState) {
    assert(gameState->initialized);

    backend_render_clearFrame(gameState->colorPallette->background);

    // pushRenderShader(&gameState->renderer, &gameState->renderer.shaders->pixelArtShader);

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
            runCalculator(gameState);
        }

    }
    float lineHeight = 5;
    float marginSpace = 5;
    float2 maxBufferSize = make_float2(0, 0);



    //NOTE: Draw radians or degrees mode
    {
        float scale = 0.8f*BODY_FONT_SCALE;
        float2 at = make_float2(0.5*plane.x, 0.5*plane.y - scale*gameState->mainFont.fontHeight);
        char *angleMode = "rad";
        if(!gameState->settingsToSave.useRadians) {
            angleMode = "deg";
        }
        Rect2f bounds = renderText(&gameState->renderer, &gameState->mainFont, angleMode, at, scale, gameState->colorPallette->standard, false);
        at.x -= get_scale_rect2f(bounds).x + 2;
        renderText(&gameState->renderer, &gameState->mainFont, angleMode, at, scale, gameState->colorPallette->standard);
    }

    //NOTE: Draw the calculator lines
    float startX = -0.5f*plane.x - gameState->bufferOffset.x;
    float2 at = make_float2(startX, -0.5f*plane.y + 5 - gameState->bufferOffset.y);
    for(int i = gameState->calculatorLinesParent.calculatorLineCount - 1; i >= getInbuiltLineCount(); --i) {
        at.x = startX;
        CalculatorLine *b = gameState->calculatorLinesParent.calculatorLines + i;

        assert(b->in);

        Rect2f dim = {};

        if(b->out) {
            dim = renderTextAsTokens(gameState, b->out, at, plane);
            // pushRenderTexture(&gameState->renderer, make_transformX_float2(get_centre_rect2f(dim), get_scale_rect2f(dim)), gameState->imageFiles.whiteImage, MY_COLOR_PASTEL_LAVENDER);
        } else {
            float2 swatchScale = make_float2(0.8f*lineHeight, 0.8f*lineHeight);
            float2 renderAt = at;
            renderAt.x += 0.5f*lineHeight;
            renderAt.y += 0.5f*lineHeight;
            pushRenderTexture(&gameState->renderer, make_transformX_float2(renderAt, swatchScale), gameState->imageFiles.whiteImage, b->colorOut);
            dim = make_rect2f_center_dim(renderAt, swatchScale);
        }

        at.x += get_scale_rect2f(dim).x + marginSpace;
        Rect2f dim1 = renderTextAsTokens(gameState, b->in, at, plane, 0.8f);

        dim = rect2f_union(dim, dim1);

        at.y += lineHeight;
        maxBufferSize.y += lineHeight;

        if(get_scale_rect2f(dim).x > maxBufferSize.x) {
            maxBufferSize.x = get_scale_rect2f(dim).x;
        }
    }
    //NOTE: Draw the input buffer
    pushRenderTexture(&gameState->renderer, make_transformX_float2(make_float2(0, -0.5f*plane.y + 0.5f*lineHeight), make_float2(plane.x, lineHeight)), gameState->imageFiles.whiteImage, gameState->colorPallette->backgroundVariation);
    if(gameState->stringBuffer.string) {
        float sideOffset = 1;
        float2 at = make_float2(-0.5f*plane.x + sideOffset, -0.5f*plane.y + 1);

        Rect2f dim = renderTextAsTokens(gameState, gameState->stringBuffer.string, at, plane, 1.0f, gameState->stringBuffer.cursor);

        if(get_scale_rect2f(dim).x > maxBufferSize.x) {
            maxBufferSize.x = get_scale_rect2f(dim).x;
        }
        maxBufferSize.y += lineHeight;
    }

    //NOTE: Render the errors
    if(gameState->currentCompilerError) {
        float yAt = -0.5f*plane.y + 1.3f*lineHeight;
        pushRenderTexture(&gameState->renderer, make_transformX_float2(make_float2(0, yAt), make_float2(plane.x, lineHeight)), gameState->imageFiles.whiteImage, gameState->colorPallette->backgroundVariation);
        float sideOffset = 1;
        float2 at = make_float2(-0.5f*plane.x + sideOffset, yAt);
        renderTextAsTokens(gameState, gameState->currentCompilerError, at, plane, 0.5f);
    }

    if(gameState->mode == INTERACTION_MODE_PICK_THEME) {
        char *names[] = {
            "handmade",
            "witness",
        	"midnight",
            "evergreen",
            "paperback",
            "cold_iron"
        };

        float2 at = make_float2(0, 0.3*plane.y);
        for(int i = 0; i < 6; ++i) {
            float scale = 0.8f*BODY_FONT_SCALE;

            char *name = names[i];
            Rect2f dim = renderText_centered(&gameState->renderer, &gameState->mainFont, name, at, scale, gameState->colorPallette->standard, false);

            float4 color = gameState->colorPallette->backgroundVariation;
            if(i == gameState->settingsToSave.themeIndex) {
                color = gameState->colorPallette->preprocessor;
            }

            pushRenderTexture(&gameState->renderer, make_transformX_float2(get_centre_rect2f(dim), get_scale_rect2f(dim)), gameState->imageFiles.whiteImage, color);
            renderText_centered(&gameState->renderer, &gameState->mainFont, name, at, scale, gameState->colorPallette->standard);

            at.y -= scale*gameState->mainFont.fontHeight;
        }
        int themeIndex;
    }


    gameState->bufferOffset = plus_float2(gameState->bufferOffset, gameState->scrollWheelDelta);
    gameState->bufferOffset.x = clamp(0, MathMaxf(0, maxBufferSize.x - plane.x), gameState->bufferOffset.x);
    gameState->bufferOffset.y = clamp(0, MathMaxf(0, maxBufferSize.y - plane.y), gameState->bufferOffset.y);


    easyUi_clearHoverId(&gameState->uiState);
}
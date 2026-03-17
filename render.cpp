void render_mergeRenderers(Renderer *a, Renderer *b) {
    if(a && b && (a->commandsCount + b->commandsCount) <= arrayCount(a->renderCommands)) {
        easyPlatform_copyMemory(a->renderCommands + a->commandsCount, b->renderCommands, sizeof(RenderItem)*b->commandsCount);
        a->commandsCount += b->commandsCount;
    } else {
        assert(false);
    }
}

void pushRenderTexture(Renderer *renderer, TransformX T, Texture *texture, float4 color = make_float4(1, 1, 1, 1)) {
    RenderItem item = {};
    assert(texture && texture->handle.handle);
    item.texture = texture;
    item.T = getModelToViewSpace_euler(T);
    item.type = RENDER_TEXTURE;
    item.color = color;
    item.uvCoords = texture->uv;
    if(renderer->commandsCount < arrayCount(renderer->renderCommands)) {
        renderer->renderCommands[renderer->commandsCount++] = item;
    } else {
        assert(false);
    }
}

void pushRenderGlyph(Renderer *renderer, float3 pos, float3 scale, float4 uvCoords, float4 color, Texture *fontTexture) {
    RenderItem item = {};
    assert(fontTexture && fontTexture->handle.handle);
    item.texture = fontTexture;
    item.T = getModelToViewSpace_euler(make_transformX(pos, scale, make_float4(0, 0, 0, 1)));
    item.type = RENDER_TEXTURE;
    item.color = color;
    item.uvCoords = uvCoords;
    if(renderer->commandsCount < arrayCount(renderer->renderCommands)) {
        renderer->renderCommands[renderer->commandsCount++] = item;
    } else {
        assert(false);
    }
}

void pushRenderRect(Renderer *renderer, TransformX T, float4 color) {
    RenderItem item = {};
    item.T = getModelToViewSpace_euler(T);
    item.type = RENDER_RECT;
    item.color = color;
    if(renderer->commandsCount < arrayCount(renderer->renderCommands)) {
        renderer->renderCommands[renderer->commandsCount++] = item;
    } else {
        assert(false);
    }
}

void pushRenderView(Renderer *renderer, float16 T) {
    if(renderer->commandsCount < arrayCount(renderer->renderCommands)) {
        RenderItem item = {};
        item.type = RENDER_VIEW_MATRIX;
        item.T = T;
        renderer->renderCommands[renderer->commandsCount++] = item;
    } else {
        assert(false);
    }
}


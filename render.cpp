void render_mergeRendererCommands(RendererCommands *a, RendererCommands *b) {
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
    item.instance.T = getModelToViewSpace_euler(T);
    item.type = RENDER_TEXTURE;
    item.instance.color = color;
    item.instance.uv = texture->uv;
    if(renderer->renderCommands.commandsCount < arrayCount(renderer->renderCommands.renderCommands)) {
        renderer->renderCommands.renderCommands[renderer->renderCommands.commandsCount++] = item;
    } else {
        assert(false);
    }
}

void pushRenderGlyph(Renderer *renderer, float3 pos, float3 scale, float4 uvCoords, float4 color, Texture *fontTexture) {
    RenderItem item = {};
    assert(fontTexture && fontTexture->handle.handle);
    item.texture = fontTexture;
    item.instance.T = getModelToViewSpace_euler(make_transformX(pos, scale, make_float4(0, 0, 0, 1)));
    item.type = RENDER_TEXTURE;
    item.instance.color = color;
    item.instance.uv = uvCoords;
    if(renderer->renderCommands.commandsCount < arrayCount(renderer->renderCommands.renderCommands)) {
        renderer->renderCommands.renderCommands[renderer->renderCommands.commandsCount++] = item;
    } else {
        assert(false);
    }
}

void pushRenderView(Renderer *renderer, float16 T) {
    if(renderer->renderCommands.commandsCount < arrayCount(renderer->renderCommands.renderCommands)) {
        RenderItem item = {};
        item.type = RENDER_VIEW_MATRIX;
        item.instance.T = T;
        renderer->renderCommands.renderCommands[renderer->renderCommands.commandsCount++] = item;
    } else {
        assert(false);
    }
}

void pushRenderShader(Renderer *renderer, Shader *shader) {
    if(renderer->renderCommands.commandsCount < arrayCount(renderer->renderCommands.renderCommands)) {
        RenderItem item = {};
        item.type = RENDER_SHADER;
        item.shader = shader;
        renderer->renderCommands.renderCommands[renderer->renderCommands.commandsCount++] = item;
    } else {
        assert(false);
    }
}


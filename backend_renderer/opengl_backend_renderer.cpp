
void backend_render_swapFrame() {
    // SDL_RenderPresent(global_sdl_renderer);
}

void backend_render_clearFrame(float4 color) {
    // SDL_SetRenderDrawColor(global_sdl_renderer, color.x*255, color.y*255, color.z*255, color.w*255);
    // SDL_RenderClear(global_sdl_renderer);    
}

void backend_render_getOutputSize(int *w, int *h) {
    // SDL_GetCurrentRenderOutputSize(global_sdl_renderer, w, h);
}

void processRenderGroup(Renderer *renderer, float2 viewPortSize) {
    float16 currentViewMatrix = float16_identity();
    for(int i = 0; i < renderer->commandsCount; ++i) {
        RenderItem *item = renderer->renderCommands + i;

        if(item->type == RENDER_RECT) {
            // SDL_SetRenderDrawColor(sdlRenderer, item->color.x*255, item->color.y*255, item->color.z*255, item->color.w*255);
            // SDL_FRect destRect = render_getDestRect(item, currentViewMatrix, viewPortSize);
            // SDL_RenderRect(sdlRenderer, &destRect);
        } else if(item->type == RENDER_TEXTURE) {
            // float4 color = make_float4(item->color.x*255, item->color.y*255, item->color.z*255, item->color.w*255);
            // SDL_SetTextureColorMod((SDL_Texture *)item->texture->handle.handle, color.x, color.y, color.z);   // red tint
            // SDL_SetTextureAlphaMod((SDL_Texture *)item->texture->handle.handle, color.w); 

            // SDL_FRect destRect = render_getDestRect(item, currentViewMatrix, viewPortSize);
            // float4 uv = item->uvCoords;
            
            // SDL_FRect srcRect = {};
            // srcRect.x = uv.x * item->texture->width;
            // srcRect.y = uv.y * item->texture->height;   
            // srcRect.w = (uv.z - uv.x) * item->texture->width;
            // srcRect.h = (uv.w - uv.y) * item->texture->height;

            // assert(item->texture->handle.handle);
            // SDL_RenderTextureRotated(sdlRenderer, (SDL_Texture *)item->texture->handle.handle, &srcRect, &destRect, 0, 0, SDL_FLIP_VERTICAL);
        } else if(item->type == RENDER_VIEW_MATRIX) {
            currentViewMatrix = item->T;
        }

    }
    renderer->commandsCount = 0;
}
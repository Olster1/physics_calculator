static SDL_Renderer* global_sdl_renderer = NULL;

float3 sdl_global_quadData[4] = {
    make_float3(-0.5f, -0.5f, 0),
    make_float3(-0.5f, 0.5f, 0),
    make_float3(0.5f, 0.5f, 0),
    make_float3(0.5f, -0.5f, 0),};


Texture *platform_loadImage(char *fileName, Arena *arena) {
    int width, height, channels;
    // Load the image pixels from disk
    unsigned char* data = stbi_load(fileName, &width, &height, &channels, 4);

    if (!data) return 0;

  if (!surface) {
      printf("IMG_Load failed: %s", SDL_GetError());
      assert(false);
      return 0;
  }

  int w = surface->w;
  int h = surface->h;

  // convert to texture
  SDL_Texture* texture = SDL_CreateTextureFromSurface(global_sdl_renderer, surface);
  SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
  SDL_DestroySurface(surface);

  SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_LINEAR);

  Texture *result = pushStruct(arena, Texture);

  result->width = w;
  result->height = h;
  result->uv = make_float4(0, 0, 1, 1);
  result->handle.handle = texture;

  return result;
}

Texture *platform_loadImageFromData(u8 *data, int w, int h, int bytesPerPixel, Arena *arena) {
  SDL_Texture* texture = SDL_CreateTexture(
      global_sdl_renderer,
      SDL_PIXELFORMAT_RGBA32,
      SDL_TEXTUREACCESS_STATIC,
      w,
      h
  );

  SDL_UpdateTexture(
      texture,
      NULL,
      data,
      w * bytesPerPixel
  );

  SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_LINEAR);
  SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);

  Texture *result = pushStruct(arena, Texture);

  result->width = w;
  result->height = h;
  result->uv = make_float4(0, 0, 1, 1);
  result->handle.handle = texture;

  return result;
}


void platform_deleteTexture(Texture *t) {
  if(t && t->handle.handle) {
    SDL_DestroyTexture((SDL_Texture *)t->handle.handle);
    t->handle.handle = 0;
  }
}

void backend_render_getOutputSize(int *w, int *h) {
    SDL_GetCurrentRenderOutputSize(global_sdl_renderer, w, h);
}

void backend_render_init(SDL_Window *hwnd, BackendRenderer *r) {
  global_sdl_renderer = SDL_CreateRenderer(window, 0);
  if (!global_sdl_renderer) {
    SDL_Log("CreateRenderer failed: %s", SDL_GetError());
    // return 1;
  }
  SDL_SetRenderVSync(global_sdl_renderer, 1);
}


void backendRenderer_setViewport(float x0, float y0, float x1, float y1) {
}

SDL_FRect render_getDestRect(RenderItem *item, float16 currentViewMatrix, float2 viewPortSize) {
    float16 T = float16_multiply(currentViewMatrix, item->T);
    float4 points[4] = {};
    for(int i = 0; i < arrayCount(sdl_global_quadData); ++i) {
        float3 p = sdl_global_quadData[i];
        points[i] = float16_transform(T, make_float4(p.x, p.y, p.z, 1));
        points[i].x /= points[i].w;
        points[i].y /= points[i].w;
        points[i].z /= points[i].w;

        points[i].x = (0.5f * (points[i].x + 1.0f)) * viewPortSize.x;
        points[i].y = (0.5f * (1.0f - points[i].y)) * viewPortSize.y;
    }

    SDL_FRect destRect = {};
    destRect.x = points[0].x;
    destRect.y = points[0].y;
    destRect.w = points[3].x - points[0].x;
    destRect.h = points[1].y - points[0].y;

    return destRect;
}

void backend_render_swapFrame(SDL_Window *hwnd) {
    SDL_RenderPresent(global_sdl_renderer);
}

void backend_render_clearFrame(float4 color) {
    SDL_SetRenderDrawColor(global_sdl_renderer, color.x*255, color.y*255, color.z*255, color.w*255);
    SDL_RenderClear(global_sdl_renderer);
}

void processRenderGroup(Renderer *renderer, float2 viewPortSize) {
    SDL_Renderer *sdlRenderer = global_sdl_renderer;
    float16 currentViewMatrix = float16_identity();
    for(int i = 0; i < renderer->commandsCount; ++i) {
        RenderItem *item = renderer->renderCommands + i;

        if(item->type == RENDER_RECT) {
            SDL_SetRenderDrawColor(sdlRenderer, item->color.x*255, item->color.y*255, item->color.z*255, item->color.w*255);
            SDL_FRect destRect = render_getDestRect(item, currentViewMatrix, viewPortSize);
            SDL_RenderRect(sdlRenderer, &destRect);
        } else if(item->type == RENDER_TEXTURE) {
            float4 color = make_float4(item->color.x*255, item->color.y*255, item->color.z*255, item->color.w*255);
            SDL_SetTextureColorMod((SDL_Texture *)item->texture->handle.handle, color.x, color.y, color.z);   // red tint
            SDL_SetTextureAlphaMod((SDL_Texture *)item->texture->handle.handle, color.w);

            SDL_FRect destRect = render_getDestRect(item, currentViewMatrix, viewPortSize);
            float4 uv = item->uvCoords;

            SDL_FRect srcRect = {};
            srcRect.x = uv.x * item->texture->width;
            srcRect.y = uv.y * item->texture->height;
            srcRect.w = (uv.z - uv.x) * item->texture->width;
            srcRect.h = (uv.w - uv.y) * item->texture->height;

            assert(item->texture->handle.handle);
            SDL_RenderTextureRotated(sdlRenderer, (SDL_Texture *)item->texture->handle.handle, &srcRect, &destRect, 0, 0, SDL_FLIP_VERTICAL);
        } else if(item->type == RENDER_VIEW_MATRIX) {
            currentViewMatrix = item->T;
        }

    }
    renderer->commandsCount = 0;
}
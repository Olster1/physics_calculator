
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3/SDL_main.h>

#include <stdbool.h>
#include "../platform.h"
#include "../easy_files.h"

static SDL_Window* window = NULL;
static int global_default_window_size_x = 1920;
static int global_default_window_size_y = 1080;

enum MouseButtonType {
  MOUSE_BUTTON_LEFT_CLICK,
  MOUSE_BUTTON_RIGHT_CLICK,
  MOUSE_BUTTON_MIDDLE_CLICK,

  MOUSE_BUTTON_TYPE_COUNT,
};

enum MouseKeyState {
  MOUSE_BUTTON_NONE,
  MOUSE_BUTTON_PRESSED,
  MOUSE_BUTTON_DOWN,
  MOUSE_BUTTON_RELEASED,
};

void platform_setWindowSize(int w, int h) {
  SDL_SetWindowSize(window, w, h);
}
void platform_setWindowPos(int w, int h) {
  SDL_SetWindowPosition(window, w, h);
}

char *platform_getClipBoardText() {
  char *result = SDL_GetClipboardText();

  return result;
}

void platform_freeClipBoardText(char *txt) {
  SDL_free(txt);
}

#include "../backend_renderer/sdl_backend_renderer.cpp"
#include "../main.cpp"

// #include "../backend_renderer/opengl_backend_renderer.cpp"

void updateMouseButton(u32 mouseState, GameState *gameState, MouseButtonType index, u32 sdlButtonType) {
  if(mouseState & SDL_BUTTON_MASK(sdlButtonType)) {
    if(gameState->mouseBtn[index] == MOUSE_BUTTON_NONE) {
      gameState->mouseBtn[index] = MOUSE_BUTTON_PRESSED;
    } else if(gameState->mouseBtn[index] == MOUSE_BUTTON_PRESSED) {
      gameState->mouseBtn[index] = MOUSE_BUTTON_DOWN;
    }
  } else {
    if(gameState->mouseBtn[index] == MOUSE_BUTTON_DOWN || gameState->mouseBtn[index] == MOUSE_BUTTON_PRESSED) {
      gameState->mouseBtn[index] = MOUSE_BUTTON_RELEASED;
    } else {
      gameState->mouseBtn[index] = MOUSE_BUTTON_NONE;
    }
  }
}

void getMouseData(GameState *gameState) {
  int w;
  int h;
  SDL_GetWindowSize(window, &w, &h);

  gameState->aspectRatioWindow_y_over_x = (float)h / (float)w;

  float x;
  float y;
  Uint32 mouseState = SDL_GetMouseState(&x, &y);
  gameState->mouseP_screenSpace.x = (float)x;
  gameState->mouseP_screenSpace.y = (float)(-y); //NOTE: Bottom corner is origin

  gameState->mouseP_01.x = gameState->mouseP_screenSpace.x / w;
  gameState->mouseP_01.y = (gameState->mouseP_screenSpace.y / h) + 1.0f;

  updateMouseButton(mouseState, gameState, MOUSE_BUTTON_LEFT_CLICK, SDL_BUTTON_LEFT);
  updateMouseButton(mouseState, gameState, MOUSE_BUTTON_MIDDLE_CLICK, SDL_BUTTON_MIDDLE);
  updateMouseButton(mouseState, gameState, MOUSE_BUTTON_RIGHT_CLICK, SDL_BUTTON_RIGHT);
}

static Uint32 lastTicks = 0;

struct UpdateLoopData {
  GameState *gameState;
  int w;
  int h;
};

static void updateFrame(void* arg) {
  UpdateLoopData *data = (UpdateLoopData *)arg;
  GameState *gameState = data->gameState;
  Uint32 now = SDL_GetTicks();
  gameState->dt = (float)(now - lastTicks) / 1000.0f;
  lastTicks = now;

  getMouseData(gameState);

  updateGame(gameState);
  processRenderGroup(&gameState->renderer, make_float2(data->w, data->h));

  backend_render_swapFrame();

}

int main(int argc, char** argv) {
  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
    SDL_Log("SDL_Init failed: %s", SDL_GetError());
    return 1;
  }

  int flags = SDL_WINDOW_RESIZABLE;

  int w = 1920;
  int h = 1080;

  window = SDL_CreateWindow("Calculator",
    w, h, flags);
  if (!window) {
    SDL_Log("CreateWindow failed: %s", SDL_GetError());
    return 1;
  }

  backend_render_init();

  backend_render_getOutputSize(&w, &h);

  platform_setWindowSize(w, h);

  global_default_window_size_x = w;
  global_default_window_size_y = h;

  //NOTE: Seed random sequence
  srand((unsigned int)time(NULL));

  initMemoryArenas();
  GameState *gameState = allocateGameState();
  gameState->aspectRatioWindow_y_over_x = (float)h / (float)w;

  lastTicks = SDL_GetTicks();

  SDL_StartTextInput(window);

  bool running = true;
  while (running) {
    SDL_Event e;
    gameState->scrollWheelDelta.x = 0;
    gameState->scrollWheelDelta.y = 0;
    while (SDL_PollEvent(&e)) {
      if (e.type == SDL_EVENT_QUIT) running = false;
      if (e.type == SDL_EVENT_MOUSE_WHEEL) {
        gameState->scrollWheelDelta.y = e.wheel.y;
        gameState->scrollWheelDelta.x = e.wheel.x;
      }
      else if (e.type == SDL_EVENT_KEY_DOWN) {
        if (e.key.key == SDLK_RETURN || e.key.key == SDLK_KP_ENTER) {
            gameState->enterPressed = true;
        }
        if(e.key.key == SDLK_V && (e.key.mod & SDL_KMOD_GUI)) {
          char *text = platform_getClipBoardText();
          stringBuffer_insertString(&gameState->stringBuffer, text);
          if(text) {
              platform_freeClipBoardText(text);
          }
        }
        if (e.key.key == SDLK_LEFT) {
          stringBuffer_cursorLeft(&gameState->stringBuffer, 1);
        }
        if (e.key.key == SDLK_RIGHT) {
          stringBuffer_cursorRight(&gameState->stringBuffer, 1);
        }
        if (e.key.key == SDLK_DOWN) {
          if(gameState->historyAt < gameState->bufferHistory.count) {
            gameState->historyAt++;
            if(gameState->historyAt < gameState->bufferHistory.count) {
              clearStringBuffer(&gameState->stringBuffer);
              stringBuffer_insertString(&gameState->stringBuffer, gameState->bufferHistory[gameState->historyAt]);
            } else {
              clearStringBuffer(&gameState->stringBuffer);
            }
          } else {
              clearStringBuffer(&gameState->stringBuffer);
          }
        }
        if (e.key.key == SDLK_UP) {
          if(gameState->historyAt > 0) {
            gameState->historyAt--;
            clearStringBuffer(&gameState->stringBuffer);
            stringBuffer_insertString(&gameState->stringBuffer, gameState->bufferHistory[gameState->historyAt]);
          }
        }
        if (e.key.key == SDLK_BACKSPACE) {
          stringBuffer_removeCharacter(&gameState->stringBuffer);
        }
      }

      if (e.type == SDL_EVENT_WINDOW_RESIZED) {
        w = e.window.data1;
        h = e.window.data2;
        gameState->aspectRatioWindow_y_over_x = (float)h / (float)w;
        gameState->settingsToSave.windowX = w;
        gameState->settingsToSave.windowY = h;

        saveSettingsFile(&gameState->settingsToSave);
      }
      if (e.type == SDL_EVENT_WINDOW_MOVED) {
        int x = e.window.data1;
        int y = e.window.data2;
        gameState->settingsToSave.windowPosX = x;
        gameState->settingsToSave.windowPosY = y;

        saveSettingsFile(&gameState->settingsToSave);
      } else if (e.type == SDL_EVENT_TEXT_INPUT) {
        stringBuffer_insertString(&gameState->stringBuffer, (char *)e.text.text);
      }

    }
    refreshPerFrameArena();

    UpdateLoopData data = {};
    data.gameState = gameState;
    data.w = w;
    data.h = h;
    updateFrame(&data);
    gameState->enterPressed = false;
  }
  SDL_Quit();

  return 0;
}
